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

    // Pattern A = default, B-H = empty (Pattern constructor fills with false/0)
    buildDefaultPattern(0);

    // Song chain: all slots point to pattern A, 1 repeat each
    for (auto& slot : songChain)
        slot = { 0, 1 };

    for (int t = 0; t < NUM_TRACKS; ++t) midiActiveNote[t] = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Default pattern: hypnotic minimal techno, A minor
// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::buildDefaultPattern(int patIdx)
{
    auto& pat = patterns[patIdx];
    for (auto& row : pat.steps)     row.fill(false);
    for (auto& row : pat.stepNotes) row.fill(0);

    // KICK — syncopated 4/4
    for (int s : { 0, 4, 10, 12 }) pat.steps[KICK][s] = true;

    // SNARE — backbeat
    pat.steps[SNARE][4]  = true;
    pat.steps[SNARE][12] = true;

    // HIHAT — eighth-note groove
    for (int s : { 1, 3, 5, 7, 9, 11, 13, 15 }) pat.steps[HIHAT][s] = true;
    pat.steps[HIHAT][0] = true;

    // BASS — Trentemøller style riff in A minor
    const int bassSteps[]  = { 0, 2, 4, 5, 6, 8, 10, 12, 14, 15 };
    const int bassDegrees[]= { 0, 0, 0, 2, 0, 0, 3,  0,  0,  4  };
    for (int i = 0; i < 10; ++i) {
        pat.steps[BASS][bassSteps[i]]     = true;
        pat.stepNotes[BASS][bassSteps[i]] = bassDegrees[i];
    }

    // LEAD — sparse ghost notes
    pat.steps[LEAD][4]  = true;  pat.stepNotes[LEAD][4]  = 0; // A3
    pat.steps[LEAD][11] = true;  pat.stepNotes[LEAD][11] = 1; // B3

    // PAD — long attack, beat 0 only
    pat.steps[PAD][0] = true;  pat.stepNotes[PAD][0] = 0; // A2
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::randomizePattern()
{
    int patIdx = editPatternIdx.load();
    auto& pat  = patterns[patIdx];

    for (auto& row : pat.steps)     row.fill(false);
    for (auto& row : pat.stepNotes) row.fill(0);

    // KICK: 4-on-the-floor + random syncopations
    for (int s : { 0, 4, 8, 12 }) pat.steps[KICK][s] = true;
    for (int s = 1; s < 16; s += 2)
        if (rng.nextFloat() > 0.82f) pat.steps[KICK][s] = true;

    // SNARE: bars 4+12 always, occasional ghost
    pat.steps[SNARE][4] = true; pat.steps[SNARE][12] = true;
    for (int s = 0; s < 16; ++s)
        if (s != 4 && s != 12 && rng.nextFloat() > 0.88f) pat.steps[SNARE][s] = true;

    // HIHAT: 8th-note or 16th-note feel
    bool sixteenth = rng.nextBool();
    for (int s = 0; s < 16; ++s)
        pat.steps[HIHAT][s] = sixteenth ? (rng.nextFloat() > 0.3f) : (s % 2 == 0);

    // BASS: sparse melodic line
    for (int s = 0; s < 16; ++s) {
        if (rng.nextFloat() > 0.55f) {
            pat.steps[BASS][s]     = true;
            pat.stepNotes[BASS][s] = rng.nextInt(7);
        }
    }

    // LEAD: very sparse
    for (int s = 0; s < 16; ++s) {
        if (rng.nextFloat() > 0.72f) {
            pat.steps[LEAD][s]     = true;
            pat.stepNotes[LEAD][s] = rng.nextInt(7);
        }
    }

    // PAD: every 8 steps
    for (int s : { 0, 8 }) {
        pat.steps[PAD][s]     = true;
        pat.stepNotes[PAD][s] = rng.nextInt(3);
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
void ObstacleProcessor::triggerStep(int step, juce::MidiBuffer& midi, int samplePos, int patIdx)
{
    const auto& pat = patterns[patIdx];
    int key = keyParam ? keyParam->get() : 0;

    static const int midiChan[NUM_TRACKS] = { 1, 2, 3, 4, 5, 6 };

    // ── MIDI output ───────────────────────────────────────────────────────────
    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        if (!pat.steps[t][step])          continue;
        if (trackMuteParam[t]->get())     continue;

        int note = 0;
        if      (t == KICK)  note = 36;
        else if (t == SNARE) note = 38;
        else if (t == HIHAT) note = 42;
        else if (t == BASS)  { int d = juce::jlimit(0,6,pat.stepNotes[BASS][step]); note = kBassBaseMidi[d] + key; }
        else if (t == LEAD)  { int d = juce::jlimit(0,6,pat.stepNotes[LEAD][step]); note = kLeadBaseMidi[d] + key; }
        else                 { int d = juce::jlimit(0,6,pat.stepNotes[PAD][step]);  note = kPadBaseMidi[d]  + key; }

        note = juce::jlimit(0, 127, note);
        int velocity = juce::jlimit(1, 127, (int)(trackVolParam[t]->get() * 100.f));

        if (midiActiveNote[t] != -1)
            midi.addEvent(juce::MidiMessage::noteOff(midiChan[t], midiActiveNote[t]), samplePos);

        midi.addEvent(juce::MidiMessage::noteOn(midiChan[t], note, (juce::uint8)velocity), samplePos);
        midiActiveNote[t] = note;
    }

    // ── Audio voices ──────────────────────────────────────────────────────────
    if (pat.steps[KICK][step])  kick.trigger();
    if (pat.steps[SNARE][step]) snare.trigger();
    if (pat.steps[HIHAT][step]) hihat.trigger(false);

    if (pat.steps[BASS][step]) {
        int deg  = juce::jlimit(0, 6, pat.stepNotes[BASS][step]);
        float freq = midiToFreq(kBassBaseMidi[deg] + key);
        bass.trigger(freq);
    }

    if (pat.steps[LEAD][step]) {
        int deg  = juce::jlimit(0, 6, pat.stepNotes[LEAD][step]);
        float freq = midiToFreq(kLeadBaseMidi[deg] + key);
        lead.trigger(freq);
    }

    if (pat.steps[PAD][step]) {
        int deg  = juce::jlimit(0, 6, pat.stepNotes[PAD][step]);
        float freq = midiToFreq(kPadBaseMidi[deg] + key);
        pad.trigger(freq);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::sendAllNotesOff(juce::MidiBuffer& midi, int samplePos)
{
    static const int midiChan[NUM_TRACKS] = { 1, 2, 3, 4, 5, 6 };

    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        if (midiActiveNote[t] != -1)
        {
            midi.addEvent(juce::MidiMessage::noteOff(midiChan[t], midiActiveNote[t]), samplePos);
            midiActiveNote[t] = -1;
        }
        midi.addEvent(juce::MidiMessage::allNotesOff(midiChan[t]), samplePos);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ObstacleProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiBuffer)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    midiBuffer.clear();

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
                // Only let host transport control playing when inside a real DAW,
                // not in standalone mode (where the UI button drives playing).
                if (wrapperType != wrapperType_Standalone)
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

    if (!playing.load())
    {
        if (wasPreviouslyPlaying)
        {
            sendAllNotesOff(midiBuffer, 0);
            wasPreviouslyPlaying = false;
        }
        return;
    }
    wasPreviouslyPlaying = true;

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    // Cache current playing pattern index for this block
    int curPatIdx = playPatternIdx.load();

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Sequencer clock (with swing) ────────────────────────────────────
        if (sampleCounter <= 0.0)
        {
            seqStep = (seqStep + 1) % 16;
            currentStep.store(seqStep);

            // ── Song chain advancement (at step 0) ───────────────────────────
            if (seqStep == 0)
            {
                loopCount++;
                int slot = playSongSlot.load();
                bool advance = nextRequested.exchange(false) || (loopCount >= songChain[slot].repeatCount);
                if (advance)
                {
                    loopCount     = 0;
                    int nextSlot  = slot + 1;
                    if (nextSlot >= songChainLength)
                    {
                        if (songLoopMode)
                            nextSlot = 0;
                        else
                        {
                            playing.store(false);
                            nextSlot = 0;
                        }
                    }
                    playSongSlot.store(nextSlot);
                    curPatIdx = songChain[nextSlot].patternIndex;
                    playPatternIdx.store(curPatIdx);
                }
            }

            triggerStep(seqStep, midiBuffer, i, curPatIdx);

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

    // 8 patterns × 6 tracks × 16 steps
    for (int p = 0; p < NUM_PATTERNS; ++p)
        for (int t = 0; t < NUM_TRACKS; ++t)
            for (int s = 0; s < 16; ++s)
                stream.writeBool(patterns[p].steps[t][s]);

    for (int p = 0; p < NUM_PATTERNS; ++p)
        for (int t = 0; t < NUM_TRACKS; ++t)
            for (int s = 0; s < 16; ++s)
                stream.writeInt(patterns[p].stepNotes[t][s]);

    // Song chain
    stream.writeInt(songChainLength);
    stream.writeBool(songLoopMode);
    for (int sl = 0; sl < NUM_SONG_SLOTS; ++sl) {
        stream.writeInt(songChain[sl].patternIndex);
        stream.writeInt(songChain[sl].repeatCount);
    }

    stream.writeInt(editPatternIdx.load());
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
        if (stream.getNumBytesRemaining() < 4) break;
        *trackVolParam[t]  = stream.readFloat();
        *trackMuteParam[t] = stream.readBool();
        *trackDecParam[t]  = stream.readFloat();
    }

    // 8 patterns × 6 tracks × 16 steps (bool)
    for (int p = 0; p < NUM_PATTERNS; ++p)
        for (int t = 0; t < NUM_TRACKS; ++t)
            for (int s = 0; s < 16; ++s)
                if (stream.getNumBytesRemaining() > 0)
                    patterns[p].steps[t][s] = stream.readBool();

    // 8 patterns × 6 tracks × 16 steps (int)
    for (int p = 0; p < NUM_PATTERNS; ++p)
        for (int t = 0; t < NUM_TRACKS; ++t)
            for (int s = 0; s < 16; ++s)
                if (stream.getNumBytesRemaining() >= 4)
                    patterns[p].stepNotes[t][s] = stream.readInt();

    // Song chain
    if (stream.getNumBytesRemaining() >= 4)
        songChainLength = juce::jlimit(1, NUM_SONG_SLOTS, stream.readInt());
    if (stream.getNumBytesRemaining() > 0)
        songLoopMode = stream.readBool();
    for (int sl = 0; sl < NUM_SONG_SLOTS; ++sl) {
        if (stream.getNumBytesRemaining() >= 8) {
            songChain[sl].patternIndex = juce::jlimit(0, NUM_PATTERNS - 1, stream.readInt());
            songChain[sl].repeatCount  = juce::jlimit(1, 8, stream.readInt());
        }
    }

    if (stream.getNumBytesRemaining() >= 4) {
        int ep = juce::jlimit(0, NUM_PATTERNS - 1, stream.readInt());
        editPatternIdx.store(ep);
    }

    // Re-init play state from slot 0
    int startSlot = 0;
    playSongSlot.store(startSlot);
    playPatternIdx.store(songChain[startSlot].patternIndex);

    bpm.store(bpmParam->get());
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ObstacleProcessor();
}
