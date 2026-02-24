#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  OBSTACLE — Sound Engine
//  6 voice types: Kick, Snare, Hihat, Bass, Lead, Pad
//  FX chain: LP filter → soft clip → dotted-8th delay → 4s reverb → compressor
// ─────────────────────────────────────────────────────────────────────────────

static constexpr float kTwoPi = 6.283185307179586f;

// ── Helpers ──────────────────────────────────────────────────────────────────

inline float softClip(float x, float drive = 1.8f)
{
    x *= drive;
    return std::tanh(x) / drive;
}

inline float lerp(float a, float b, float t) { return a + t * (b - a); }

// ── One-pole LP filter ────────────────────────────────────────────────────────
struct OnePoleLP
{
    float s = 0.f;
    float process(float in, float cutoff01)
    {
        float c = juce::jlimit(0.0001f, 0.9999f, cutoff01);
        s += c * (in - s);
        return s;
    }
    void reset() { s = 0.f; }
};

// ── ADSR envelope ─────────────────────────────────────────────────────────────
struct Env
{
    enum Phase { Idle, Attack, Decay, Sustain, Release };
    Phase phase = Idle;
    float val = 0.f, sr = 44100.f;
    float a = 0.001f, d = 0.1f, su = 0.5f, r = 0.1f;   // times in seconds
    bool triggered = false;

    void setSampleRate(float s) { sr = s; }
    void setADSR(float att, float dec, float sus, float rel)
    { a = att; d = dec; su = sus; r = rel; }

    void trigger()  { phase = Attack; val = 0.f; triggered = true; }
    void release()  { if (phase != Idle) phase = Release; }
    bool isActive() const { return phase != Idle; }

    float tick()
    {
        switch (phase)
        {
            case Attack:
                val += 1.f / (a * sr);
                if (val >= 1.f) { val = 1.f; phase = Decay; }
                break;
            case Decay:
                val -= (1.f - su) / (d * sr);
                if (val <= su) { val = su; phase = Sustain; }
                break;
            case Sustain:
                break;
            case Release:
                val -= su / (r * sr);
                if (val <= 0.f) { val = 0.f; phase = Idle; }
                break;
            default: break;
        }
        return val;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  KICK VOICE
//  click transient 1200 Hz + sub sine sweep 180→28 Hz + noise thump
// ═════════════════════════════════════════════════════════════════════════════
struct KickVoice
{
    float sr = 44100.f;
    float subPhase = 0.f;
    float clickPhase = 0.f;
    float envSub = 0.f;
    float envClick = 0.f;
    float envNoise = 0.f;
    float noiseLP = 0.f;
    bool active = false;
    float t = 0.f;

    // Settable
    float subDecayTime = 0.40f;
    void setDecay(float d) { subDecayTime = juce::jlimit(0.10f, 1.50f, d); }

    juce::Random rng;

    void prepare(float sampleRate) { sr = sampleRate; }

    void trigger()
    {
        active = true;
        t = 0.f;
        subPhase = 0.f;
        clickPhase = 0.f;
        envSub = 1.f;
        envClick = 1.f;
        envNoise = 1.f;
        noiseLP = 0.f;
    }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;

        // sub sine sweep 180 → 28 Hz over 300ms
        float sweepTime = 0.30f;
        float freq = lerp(180.f, 28.f, juce::jlimit(0.f, 1.f, t / sweepTime));
        subPhase += kTwoPi * freq * dt;
        if (subPhase > kTwoPi) subPhase -= kTwoPi;
        float subOut = std::sin(subPhase) * envSub;

        // click transient 1200 Hz, decay 8ms
        clickPhase += kTwoPi * 1200.f * dt;
        if (clickPhase > kTwoPi) clickPhase -= kTwoPi;
        float clickOut = std::sin(clickPhase) * envClick * 0.7f;

        // noise thump through one-pole LP, decay 40ms
        float noise = rng.nextFloat() * 2.f - 1.f;
        noiseLP += 0.15f * (noise - noiseLP);
        float noiseOut = noiseLP * envNoise * 0.4f;

        // envelopes
        envSub   -= dt / subDecayTime;
        envClick -= dt / 0.008f;
        envNoise -= dt / 0.04f;

        envSub   = juce::jmax(0.f, envSub);
        envClick = juce::jmax(0.f, envClick);
        envNoise = juce::jmax(0.f, envNoise);

        t += dt;
        if (envSub <= 0.f) active = false;

        return (subOut + clickOut + noiseOut) * 0.6f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  SNARE VOICE
//  noise HPF/bandpass + tone with pitch drop
// ═════════════════════════════════════════════════════════════════════════════
struct SnareVoice
{
    float sr = 44100.f;
    float tonePhase = 0.f;
    float envTone = 0.f;
    float envNoise = 0.f;
    bool active = false;
    float t = 0.f;

    // simple 2-pole HPF state
    float hp1 = 0.f, hp2 = 0.f;
    float bp1 = 0.f, bp2 = 0.f;

    // Settable
    float noiseDecayTime = 0.18f;
    void setDecay(float d) { noiseDecayTime = juce::jlimit(0.05f, 0.50f, d); }

    juce::Random rng;

    void prepare(float sampleRate) { sr = sampleRate; }

    void trigger()
    {
        active = true;
        t = 0.f;
        tonePhase = 0.f;
        envTone = 1.f;
        envNoise = 1.f;
        hp1 = hp2 = bp1 = bp2 = 0.f;
    }

    // Simple 2-pole HPF (Chamberlin state variable)
    float hpf(float in, float cutHz)
    {
        float f = 2.f * std::sin(juce::MathConstants<float>::pi * cutHz / sr);
        float q = 1.4f;
        float lp = bp2 + f * hp2;
        float high = in - lp - q * bp2;
        float band = f * high + bp2;
        bp2 = band; hp2 = high;
        return high;
    }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;

        // tone: starts at 220 Hz drops to 80 Hz over 60ms
        float freq = lerp(220.f, 80.f, juce::jlimit(0.f, 1.f, t / 0.06f));
        tonePhase += kTwoPi * freq * dt;
        if (tonePhase > kTwoPi) tonePhase -= kTwoPi;
        float toneOut = std::sin(tonePhase) * envTone * 0.5f;

        // noise through HPF at 1200 Hz
        float noise = rng.nextFloat() * 2.f - 1.f;
        float noiseHP = hpf(noise, 1200.f);
        float noiseOut = noiseHP * envNoise * 0.6f;

        envTone  -= dt / 0.12f;
        envNoise -= dt / noiseDecayTime;
        envTone  = juce::jmax(0.f, envTone);
        envNoise = juce::jmax(0.f, envNoise);

        t += dt;
        if (envNoise <= 0.f) active = false;

        return (toneOut + noiseOut) * 0.55f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  HIHAT VOICE
//  5 detuned square oscillators + noise HPF 9kHz+
// ═════════════════════════════════════════════════════════════════════════════
struct HihatVoice
{
    float sr = 44100.f;
    std::array<float, 5> phases{};
    float env = 0.f;
    bool active = false;
    bool isOpen = false;

    // HPF state for noise
    float hpState = 0.f;
    juce::Random rng;

    // base freqs in "metallic" ratio
    const std::array<float, 5> freqMults = { 1.0f, 1.483f, 1.727f, 2.017f, 2.278f };
    static constexpr float baseFreq = 3200.f;

    // Settable
    float chDecayTime = 0.06f;
    void setDecay(float d) { chDecayTime = juce::jlimit(0.01f, 0.30f, d); }

    void prepare(float sampleRate) { sr = sampleRate; }

    void trigger(bool open = false)
    {
        active = true;
        isOpen = open;
        env = 1.f;
        hpState = 0.f;
        for (auto& p : phases) p = 0.f;
    }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;
        float out = 0.f;

        // 5 detuned square oscillators
        for (int i = 0; i < 5; ++i)
        {
            phases[i] += kTwoPi * baseFreq * freqMults[i] * dt;
            if (phases[i] > kTwoPi) phases[i] -= kTwoPi;
            out += (phases[i] < juce::MathConstants<float>::pi ? 1.f : -1.f);
        }
        out /= 5.f;
        out *= 0.5f;

        // noise HPF at 9kHz
        float noise = rng.nextFloat() * 2.f - 1.f;
        float alpha = 1.f - std::exp(-kTwoPi * 9000.f / sr);
        hpState += alpha * (noise - hpState);
        float noiseHP = noise - hpState;
        out += noiseHP * 0.4f;

        out *= env;

        float decayTime = isOpen ? 0.35f : chDecayTime;
        env -= dt / decayTime;
        if (env <= 0.f) { env = 0.f; active = false; }

        return out * 0.35f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  BASS VOICE  (Trentemøller style)
//  2x detuned sawtooth + sub sine + filter envelope opens→closes
// ═════════════════════════════════════════════════════════════════════════════
struct BassVoice
{
    float sr = 44100.f;
    float phase1 = 0.f, phase2 = 0.f, subPhase = 0.f;
    float envAmp = 0.f;
    float filterEnv = 0.f;
    float filterState = 0.f;
    bool active = false;
    float t = 0.f;
    float noteFreq = 55.f;

    // Settable: 0=dark/closed, 1=full brightness
    float filterOpenAmt = 1.0f;
    void setFilterOpen(float v) { filterOpenAmt = juce::jlimit(0.f, 1.f, v); }

    void prepare(float sampleRate) { sr = sampleRate; }

    void trigger(float freq = 55.f)
    {
        active = true;
        noteFreq = freq;
        t = 0.f;
        phase1 = phase2 = subPhase = 0.f;
        envAmp = 1.f;
        filterEnv = 0.f;
        filterState = 0.f;
    }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;
        float detuneHz = noteFreq * 0.012f;

        phase1 += kTwoPi * noteFreq * dt;
        phase2 += kTwoPi * (noteFreq + detuneHz) * dt;
        if (phase1 > kTwoPi) phase1 -= kTwoPi;
        if (phase2 > kTwoPi) phase2 -= kTwoPi;

        float saw1 = phase1 / juce::MathConstants<float>::pi - 1.f;
        float saw2 = phase2 / juce::MathConstants<float>::pi - 1.f;

        subPhase += kTwoPi * (noteFreq * 0.5f) * dt;
        if (subPhase > kTwoPi) subPhase -= kTwoPi;
        float sub = std::sin(subPhase) * 0.6f;

        float rawOut = (saw1 + saw2) * 0.4f + sub;

        float attackTime  = 0.02f;
        float decayTime   = 0.22f;
        if (t < attackTime)
            filterEnv = t / attackTime;
        else
            filterEnv = juce::jmax(0.f, 1.f - (t - attackTime) / decayTime);

        // cutoff range scaled by filterOpenAmt
        float cutNorm = 0.003f + filterEnv * (filterOpenAmt * 0.18f);
        filterState += cutNorm * (rawOut - filterState);
        float filtOut = filterState;

        filtOut *= envAmp;

        float holdTime = 0.25f;
        float releaseTime = 0.3f;
        if (t > holdTime)
            envAmp = juce::jmax(0.f, 1.f - (t - holdTime) / releaseTime);

        t += dt;
        if (envAmp <= 0.f) active = false;

        return filtOut * 0.7f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  LEAD VOICE
//  2x micro-detuned sawtooth + 0.8 Hz vibrato LFO + square sub-octave
// ═════════════════════════════════════════════════════════════════════════════
struct LeadVoice
{
    float sr = 44100.f;
    float phase1 = 0.f, phase2 = 0.f, subPhase = 0.f;
    float lfoPhase = 0.f;
    Env ampEnv;
    bool active = false;
    float noteFreq = 220.f;

    // Settable
    void setAttack(float a) { ampEnv.a = juce::jlimit(0.001f, 0.50f, a); }

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        ampEnv.setSampleRate(sampleRate);
        ampEnv.setADSR(0.12f, 0.1f, 0.7f, 0.4f);
    }

    void trigger(float freq = 220.f)
    {
        active = true;
        noteFreq = freq;
        phase1 = phase2 = subPhase = 0.f;
        ampEnv.trigger();
    }

    void noteOff() { ampEnv.release(); }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;

        lfoPhase += kTwoPi * 0.8f * dt;
        if (lfoPhase > kTwoPi) lfoPhase -= kTwoPi;
        float lfo = std::sin(lfoPhase) * 4.f;

        float f1 = noteFreq + lfo;
        float f2 = noteFreq * 1.003f + lfo;

        phase1 += kTwoPi * f1 * dt;
        phase2 += kTwoPi * f2 * dt;
        if (phase1 > kTwoPi) phase1 -= kTwoPi;
        if (phase2 > kTwoPi) phase2 -= kTwoPi;

        float saw1 = phase1 / juce::MathConstants<float>::pi - 1.f;
        float saw2 = phase2 / juce::MathConstants<float>::pi - 1.f;

        subPhase += kTwoPi * (noteFreq * 0.5f) * dt;
        if (subPhase > kTwoPi) subPhase -= kTwoPi;
        float sq = (subPhase < juce::MathConstants<float>::pi) ? 0.5f : -0.5f;

        float env = ampEnv.tick();
        if (!ampEnv.isActive()) active = false;

        return ((saw1 + saw2) * 0.4f + sq * 0.25f) * env * 0.55f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  PAD VOICE
//  4-voice detuned sine cluster, slow attack
// ═════════════════════════════════════════════════════════════════════════════
struct PadVoice
{
    float sr = 44100.f;
    std::array<float, 4> phases{};
    Env ampEnv;
    bool active = false;
    float noteFreq = 110.f;

    const std::array<float, 4> detunes = { 0.998f, 1.000f, 1.002f, 1.004f };

    // Settable
    void setAttack(float a) { ampEnv.a = juce::jlimit(0.05f, 5.0f, a); }

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        ampEnv.setSampleRate(sampleRate);
        ampEnv.setADSR(1.5f, 0.5f, 0.6f, 2.0f);
    }

    void trigger(float freq = 110.f)
    {
        active = true;
        noteFreq = freq;
        for (auto& p : phases) p = 0.f;
        ampEnv.trigger();
    }

    void noteOff() { ampEnv.release(); }

    float process()
    {
        if (!active) return 0.f;

        float dt = 1.f / sr;
        float out = 0.f;

        for (int i = 0; i < 4; ++i)
        {
            phases[i] += kTwoPi * noteFreq * detunes[i] * dt;
            if (phases[i] > kTwoPi) phases[i] -= kTwoPi;
            out += std::sin(phases[i]);
        }
        out /= 4.f;

        float env = ampEnv.tick();
        if (!ampEnv.isActive()) active = false;

        return out * env * 0.5f;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  FX CHAIN
//  LP filter → soft clip → dotted-8th delay → 4s reverb → compressor
// ═════════════════════════════════════════════════════════════════════════════
class FXChain
{
public:
    // ── Settable parameters ────────────────────────────────────────────────
    float lpCutHz      = 8000.f;
    float reverbMixAmt = 0.40f;
    float delayMixAmt  = 0.35f;
    float delayFbk     = 0.42f;
    float driveAmt     = 1.4f;

    void setLPCutoff     (float hz) { lpCutHz      = juce::jlimit(200.f, 20000.f, hz); }
    void setReverbMix    (float m)  { reverbMixAmt = juce::jlimit(0.f,   1.f,     m);  }
    void setDelayMix     (float m)  { delayMixAmt  = juce::jlimit(0.f,   0.9f,    m);  }
    void setDelayFeedback(float f)  { delayFbk     = juce::jlimit(0.f,   0.9f,    f);  }
    void setDrive        (float d)  { driveAmt     = juce::jlimit(0.5f,  10.f,    d);  }

    void prepare(float sampleRate, float bpm)
    {
        sr = sampleRate;
        updateDelayTime(bpm);

        for (int i = 0; i < 4; ++i)
        {
            combDelay[i].resize((int)(combTimes[i] * sr) + 1, 0.f);
            combIdx[i] = 0;
        }
        for (int i = 0; i < 2; ++i)
        {
            apDelay[i].resize((int)(apTimes[i] * sr) + 1, 0.f);
            apIdx[i] = 0;
        }

        delayBuf.assign(int(sr * 2.0f), 0.f);
        delayIdx = 0;

        lpState = 0.f;
        rmsState = 0.f;
        gainState = 1.f;
    }

    void updateDelayTime(float bpm)
    {
        float beat = 60.f / bpm;
        delaySamples = int((beat * 0.75f) * sr);
    }

    float process(float in)
    {
        // ── 1. LP filter ────────────────────────────────────────────────────
        float alpha = 1.f - std::exp(-kTwoPi * lpCutHz / sr);
        lpState += alpha * (in - lpState);
        float x = lpState;

        // ── 2. Soft clip (drive parameter) ─────────────────────────────────
        x = softClip(x, driveAmt);

        // ── 3. Dotted-8th delay ─────────────────────────────────────────────
        int dLen = (int)delayBuf.size();
        int readIdx = (delayIdx - juce::jlimit(1, dLen - 1, delaySamples) + dLen) % dLen;
        float delayOut = delayBuf[readIdx];
        delayBuf[delayIdx] = x + delayOut * delayFbk;
        delayIdx = (delayIdx + 1) % dLen;
        x = x * 0.7f + delayOut * delayMixAmt;

        // ── 4. Schroeder reverb (4 comb + 2 allpass) ────────────────────────
        float dryPreRev = x;
        float combOut = 0.f;
        for (int i = 0; i < 4; ++i)
        {
            int len = (int)combDelay[i].size();
            float y = combDelay[i][combIdx[i]];
            combDelay[i][combIdx[i]] = x + y * combG[i];
            combIdx[i] = (combIdx[i] + 1) % len;
            combOut += y;
        }
        combOut *= 0.25f;

        for (int i = 0; i < 2; ++i)
        {
            int len = (int)apDelay[i].size();
            float y = apDelay[i][apIdx[i]];
            float w = combOut + y * (-0.5f);
            apDelay[i][apIdx[i]] = combOut + y * 0.5f;
            apIdx[i] = (apIdx[i] + 1) % len;
            combOut = y + w * 0.5f;
        }

        x = dryPreRev * (1.f - reverbMixAmt) + combOut * reverbMixAmt;

        // ── 5. Simple RMS compressor ─────────────────────────────────────────
        float rmsTC  = std::exp(-1.f / (0.05f * sr));
        float gainTC = std::exp(-1.f / (0.1f * sr));
        rmsState = rmsTC * rmsState + (1.f - rmsTC) * x * x;
        float rmsVal = std::sqrt(rmsState + 1e-9f);
        float threshold = 0.5f;
        float ratio = 4.f;
        float desiredGain = 1.f;
        if (rmsVal > threshold)
            desiredGain = threshold / rmsVal * (1.f + (rmsVal / threshold - 1.f) / ratio);
        gainState = gainTC * gainState + (1.f - gainTC) * desiredGain;
        x *= gainState * 1.8f;

        return juce::jlimit(-1.f, 1.f, x);
    }

private:
    float sr = 44100.f;
    float lpState = 0.f;

    std::vector<float> delayBuf;
    int delayIdx = 0;
    int delaySamples = 0;

    const std::array<float, 4> combTimes = { 0.0297f, 0.0371f, 0.0411f, 0.0437f };
    const std::array<float, 4> combG     = { 0.805f,  0.827f,  0.783f,  0.764f  };
    std::array<std::vector<float>, 4> combDelay;
    std::array<int, 4> combIdx{};

    const std::array<float, 2> apTimes = { 0.0090f, 0.0061f };
    std::array<std::vector<float>, 2> apDelay;
    std::array<int, 2> apIdx{};

    float rmsState = 0.f;
    float gainState = 1.f;
};
