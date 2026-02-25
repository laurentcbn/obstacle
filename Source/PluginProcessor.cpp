#include "PluginProcessor.h"
#include "PluginEditor.h"

ObstacleProcessor::ObstacleProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // ── Global ──────────────────────────────────────────────────────────────
    addParameter (bpmParam = new juce::AudioParameterFloat (
        "bpm", "BPM",
        juce::NormalisableRange<float>(60.f, 200.f, 0.1f), 128.f));

    addParameter (masterVolParam = new juce::AudioParameterFloat (
        "master_vol", "Master Volume",
        juce::NormalisableRange<float>(0.f, 1.5f, 0.01f), 1.0f));

    addParameter (reverbParam = new juce::AudioParameterFloat (
        "fx_rev", "Reverb",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.40f));

    addParameter (delayMixParam = new juce::AudioParameterFloat (
        "fx_dly", "Delay Mix",
        juce::NormalisableRange<float>(0.f, 0.9f, 0.01f), 0.35f));

    addParameter (delayFeedParam = new juce::AudioParameterFloat (
        "fx_fbd", "Delay Feedback",
        juce::NormalisableRange<float>(0.f, 0.9f, 0.01f), 0.42f));

    addParameter (filterCutParam = new juce::AudioParameterFloat (
        "fx_cut", "Filter Cutoff",
        juce::NormalisableRange<float>(500.f, 20000.f, 10.f), 8000.f));

    addParameter (swingParam = new juce::AudioParameterFloat (
        "fx_swg", "Swing",
        juce::NormalisableRange<float>(0.f, 0.30f, 0.005f), 0.0f));

    addParameter (driveParam = new juce::AudioParameterFloat (
        "fx_drv", "Drive",
        juce::NormalisableRange<float>(0.5f, 10.f, 0.1f), 1.4f));

    addParameter (keyParam = new juce::AudioParameterInt (
        "key", "Key Transpose", -12, 12, 0));

    // ── Per-track ────────────────────────────────────────────────────────────
    static const char* ids[]   = { "kick","snare","hihat","bass","lead","pad" };
    static const char* names[] = { "Kick","Snare","Hihat","Bass","Lead","Pad" };

    // decay param ranges per track: {min, max, default}
    static const float decRange[NUM_TRACKS][3] = {
        { 0.10f, 1.50f, 0.40f },   // KICK  : sub decay
        { 0.05f, 0.50f, 0.18f },   // SNARE : noise decay
        { 0.01f, 0.30f, 0.06f },   // HIHAT : closed decay
        { 0.00f, 1.00f, 0.80f },   // BASS  : filter openness
        { 0.01f, 0.50f, 0.12f },   // LEAD  : attack
        { 0.10f, 5.00f, 1.50f },   // PAD   : attack
    };

    static const char* decIds[]   = { "kick_dec","snare_dec","hihat_dec","bass_filt","lead_atk","pad_atk" };
    static const char* decNames[] = { "Kick Decay","Snare Decay","Hihat Decay","Bass Filter","Lead Attack","Pad Attack" };

    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        addParameter (trackVolParam[t] = new juce::AudioParameterFloat (
            juce::String(ids[t]) + "_vol",
            juce::String(names[t]) + " Volume",
            juce::NormalisableRange<float>(0.f, 1.5f, 0.01f), 1.0f));

        addParameter (trackMuteParam[t] = new juce::AudioParameterBool (
            juce::String(ids[t]) + "_mute",
            juce::String(names[t]) + " Mute",
            false));

        addParameter (trackDecParam[t] = new juce::AudioParameterFloat (
            decIds[t], decNames[t],
            juce::NormalisableRange<float>(decRange[t][0], decRange[t][1], 0.001f),
            decRange[t][2]));
    }

    buildDefaultPattern();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Default pattern: hypnotic minimal techno, A minor
// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::buildDefaultPattern()
{
    for (auto& row : steps)     row.fill(false);
    for (auto& row : stepNotes) row.fill(0);

    // KICK — syncopated 4/4
    for (int s : { 0, 4, 10, 12 }) steps[KICK][s] = true;

    // SNARE — backbeat
    steps[SNARE][4]  = true;
    steps[SNARE][12] = true;

    // HIHAT — eighth-note groove
    for (int s : { 1, 3, 5, 7, 9, 11, 13, 15 }) steps[HIHAT][s] = true;
    steps[HIHAT][0] = true;

    // BASS — Trentemøller style riff in A minor
    // Original kBassFreqs mapped to scale degrees:
    // A1=55Hz→deg0, C2=65.4Hz→deg2, D2=73.4Hz→deg3, E2=82.4Hz→deg4
    const int bassSteps[]  = { 0, 2, 4, 5, 6, 8, 10, 12, 14, 15 };
    const int bassDegrees[]= { 0, 0, 0, 2, 0, 0, 3,  0,  0,  4  };
    for (int i = 0; i < 10; ++i) {
        steps[BASS][bassSteps[i]]    = true;
        stepNotes[BASS][bassSteps[i]] = bassDegrees[i];
    }

    // LEAD — sparse ghost notes
    steps[LEAD][4]  = true;  stepNotes[LEAD][4]  = 0; // A3
    steps[LEAD][11] = true;  stepNotes[LEAD][11] = 1; // B3

    // PAD — long attack, beat 0 only
    steps[PAD][0] = true;  stepNotes[PAD][0] = 0; // A2
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::randomizePattern()
{
    for (auto& row : steps)     row.fill(false);
    for (auto& row : stepNotes) row.fill(0);

    // KICK: 4-on-the-floor + random syncopations
    for (int s : { 0, 4, 8, 12 }) steps[KICK][s] = true;
    for (int s = 1; s < 16; s += 2)
        if (rng.nextFloat() > 0.82f) steps[KICK][s] = true;

    // SNARE: bars 4+12 always, occasional ghost
    steps[SNARE][4] = true; steps[SNARE][12] = true;
    for (int s = 0; s < 16; ++s)
        if (s != 4 && s != 12 && rng.nextFloat() > 0.88f) steps[SNARE][s] = true;

    // HIHAT: 8th-note or 16th-note feel
    bool sixteenth = rng.nextBool();
    for (int s = 0; s < 16; ++s)
        steps[HIHAT][s] = sixteenth ? (rng.nextFloat() > 0.3f) : (s % 2 == 0);

    // BASS: sparse melodic line
    for (int s = 0; s < 16; ++s) {
        if (rng.nextFloat() > 0.55f) {
            steps[BASS][s] = true;
            stepNotes[BASS][s] = rng.nextInt(7);
        }
    }

    // LEAD: very sparse
    for (int s = 0; s < 16; ++s) {
        if (rng.nextFloat() > 0.72f) {
            steps[LEAD][s] = true;
            stepNotes[LEAD][s] = rng.nextInt(7);
        }
    }

    // PAD: every 8 steps
    for (int s : { 0, 8 }) {
        steps[PAD][s] = true;
        stepNotes[PAD][s] = rng.nextInt(3); // A, B, or C
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr = (float)sampleRate;

    kick.prepare(sr);
    snare.prepare(sr);
    hihat.prepare(sr);
    bass.prepare(sr);
    lead.prepare(sr);
    pad.prepare(sr);

    fx.prepare(sr, bpmParam->get());
    updateStepTiming();

    sampleCounter = 0.0;
    seqStep = 0;
}

void ObstacleProcessor::updateStepTiming()
{
    float currentBpm = bpmParam->get();
    double stepSecs = 60.0 / currentBpm / 4.0;
    samplesPerStep = stepSecs * sr;
    fx.updateDelayTime(currentBpm);
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::triggerStep(int step)
{
    int key = keyParam ? keyParam->get() : 0;

    if (steps[KICK][step])  kick.trigger();
    if (steps[SNARE][step]) snare.trigger();
    if (steps[HIHAT][step]) hihat.trigger(false);

    if (steps[BASS][step]) {
        int deg  = juce::jlimit(0, 6, stepNotes[BASS][step]);
        float freq = midiToFreq(kBassBaseMidi[deg] + key);
        bass.trigger(freq);
    }

    if (steps[LEAD][step]) {
        int deg  = juce::jlimit(0, 6, stepNotes[LEAD][step]);
        float freq = midiToFreq(kLeadBaseMidi[deg] + key);
        lead.trigger(freq);
    }

    if (steps[PAD][step]) {
        int deg  = juce::jlimit(0, 6, stepNotes[PAD][step]);
        float freq = midiToFreq(kPadBaseMidi[deg] + key);
        pad.trigger(freq);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ── Host transport sync (GarageBand / Logic Pro) ─────────────────────────
    if (auto* playHead = getPlayHead())
    {
        const auto pos = playHead->getPosition();
        if (pos.hasValue())
        {
            // Sync BPM from host
            if (const auto hostBpm = pos->getBpm())
            {
                const float hbpm = juce::jlimit(60.f, 200.f, (float)*hostBpm);
                if (std::abs(hbpm - bpm.load()) > 0.05f)
                {
                    bpm.store(hbpm);
                    *bpmParam = hbpm;
                    updateStepTiming();
                }
            }

            // Sync play/stop from host
            {
                const bool hostPlaying = pos->getIsPlaying();
                if (hostPlaying && !wasHostPlaying)
                {
                    // Host just started — align sequencer to PPQ grid
                    if (const auto ppq = pos->getPpqPosition())
                    {
                        const double stepsF = *ppq / 0.25; // 16th notes
                        seqStep = (int)std::floor(stepsF) % 16;
                        const double frac = stepsF - std::floor(stepsF);
                        sampleCounter = samplesPerStep * (1.0 - frac);
                    }
                    else
                    {
                        seqStep = -1;
                        sampleCounter = 0.0;
                    }
                }
                playing.store(hostPlaying);
                wasHostPlaying = hostPlaying;
            }
        }
    }

    // ── Sync BPM ────────────────────────────────────────────────────────────
    float newBpm = bpmParam->get();
    if (std::abs(newBpm - bpm.load()) > 0.05f) {
        bpm.store(newBpm);
        updateStepTiming();
    }

    // ── Apply voice parameters ───────────────────────────────────────────────
    kick.setDecay     (trackDecParam[KICK]->get());
    snare.setDecay    (trackDecParam[SNARE]->get());
    hihat.setDecay    (trackDecParam[HIHAT]->get());
    bass.setFilterOpen(trackDecParam[BASS]->get());
    lead.setAttack    (trackDecParam[LEAD]->get());
    pad.setAttack     (trackDecParam[PAD]->get());

    // ── Apply FX parameters ──────────────────────────────────────────────────
    fx.setLPCutoff     (filterCutParam->get());
    fx.setReverbMix    (reverbParam->get());
    fx.setDelayMix     (delayMixParam->get());
    fx.setDelayFeedback(delayFeedParam->get());
    fx.setDrive        (driveParam->get());

    float masterVol = masterVolParam->get();
    double swingAmt = (double)swingParam->get();

    // ── Cache per-track gains ────────────────────────────────────────────────
    float trackGains[NUM_TRACKS];
    for (int t = 0; t < NUM_TRACKS; ++t)
        trackGains[t] = trackMuteParam[t]->get() ? 0.f : trackVolParam[t]->get();

    if (!playing.load()) return;

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Sequencer clock (with swing) ────────────────────────────────────
        if (sampleCounter <= 0.0)
        {
            seqStep = (seqStep + 1) % 16;
            currentStep.store(seqStep);
            triggerStep(seqStep);
            // Swing: alternate step length (even=longer, odd=shorter)
            double swingFactor = (seqStep % 2 == 0) ? (1.0 + swingAmt) : (1.0 - swingAmt);
            sampleCounter += samplesPerStep * swingFactor;
        }
        --sampleCounter;

        // ── Sum voices with per-track gain ──────────────────────────────────
        float mono = kick.process()  * trackGains[KICK]
                   + snare.process() * trackGains[SNARE]
                   + hihat.process() * trackGains[HIHAT]
                   + bass.process()  * trackGains[BASS]
                   + lead.process()  * trackGains[LEAD]
                   + pad.process()   * trackGains[PAD];

        float wet = fx.process(mono * masterVol);

        outL[i] = wet;
        outR[i] = wet;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* ObstacleProcessor::createEditor()
{
    return new ObstacleEditor (*this);
}

void ObstacleProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream stream (dest, true);

    // BPM + all float/bool params
    stream.writeFloat(bpmParam->get());
    stream.writeFloat(masterVolParam->get());
    stream.writeFloat(reverbParam->get());
    stream.writeFloat(delayMixParam->get());
    stream.writeFloat(delayFeedParam->get());
    stream.writeFloat(filterCutParam->get());
    stream.writeFloat(swingParam->get());
    stream.writeFloat(driveParam->get());
    stream.writeInt(keyParam->get());

    for (int t = 0; t < NUM_TRACKS; ++t) {
        stream.writeFloat(trackVolParam[t]->get());
        stream.writeBool(trackMuteParam[t]->get());
        stream.writeFloat(trackDecParam[t]->get());
    }

    // Steps + stepNotes
    for (int t = 0; t < NUM_TRACKS; ++t)
        for (int s = 0; s < 16; ++s)
            stream.writeBool(steps[t][s]);

    for (int t = 0; t < NUM_TRACKS; ++t)
        for (int s = 0; s < 16; ++s)
            stream.writeInt(stepNotes[t][s]);
}

void ObstacleProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t)sizeInBytes, false);
    if (stream.getNumBytesRemaining() < 4) return;

    *bpmParam       = stream.readFloat();
    *masterVolParam = stream.readFloat();
    *reverbParam    = stream.readFloat();
    *delayMixParam  = stream.readFloat();
    *delayFeedParam = stream.readFloat();
    *filterCutParam = stream.readFloat();
    *swingParam     = stream.readFloat();
    *driveParam     = stream.readFloat();
    *keyParam       = stream.readInt();

    for (int t = 0; t < NUM_TRACKS; ++t) {
        *trackVolParam[t]  = stream.readFloat();
        *trackMuteParam[t] = stream.readBool();
        *trackDecParam[t]  = stream.readFloat();
    }

    for (int t = 0; t < NUM_TRACKS; ++t)
        for (int s = 0; s < 16; ++s)
            if (stream.getNumBytesRemaining() > 0)
                steps[t][s] = stream.readBool();

    for (int t = 0; t < NUM_TRACKS; ++t)
        for (int s = 0; s < 16; ++s)
            if (stream.getNumBytesRemaining() >= 4)
                stepNotes[t][s] = stream.readInt();

    bpm.store(bpmParam->get());
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ObstacleProcessor();
}
