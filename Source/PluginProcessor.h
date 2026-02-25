#pragma once
#include <JuceHeader.h>
#include "SynthEngine.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Track indices
// ─────────────────────────────────────────────────────────────────────────────
enum TrackID { KICK = 0, SNARE, HIHAT, BASS, LEAD, PAD, NUM_TRACKS };

// A-natural minor scale, per voice type — MIDI base notes
// stepNotes[track][step] = 0..6 → index into these arrays
static constexpr std::array<int, 7> kBassBaseMidi = { 33, 35, 36, 38, 40, 41, 43 }; // A1 to G2
static constexpr std::array<int, 7> kLeadBaseMidi = { 57, 59, 60, 62, 64, 65, 67 }; // A3 to G4
static constexpr std::array<int, 7> kPadBaseMidi  = { 45, 47, 48, 50, 52, 53, 55 }; // A2 to G3

static const char* kNoteNames[] = { "A", "B", "C", "D", "E", "F", "G" };

inline float midiToFreq(int midi) {
    return 440.f * std::pow(2.f, (midi - 69) / 12.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Song Mode structures
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int NUM_PATTERNS   = 8;
static constexpr int NUM_SONG_SLOTS = 16;

struct Pattern {
    std::array<std::array<bool, 16>, NUM_TRACKS> steps;
    std::array<std::array<int,  16>, NUM_TRACKS> stepNotes;
    Pattern() {
        for (auto& r : steps)     r.fill(false);
        for (auto& r : stepNotes) r.fill(0);
    }
};

struct SongSlot {
    int patternIndex = 0;  // 0-7 (A-H)
    int repeatCount  = 1;  // 1-8 loops before advancing
};

// ─────────────────────────────────────────────────────────────────────────────
class ObstacleProcessor  : public juce::AudioProcessor
{
public:
    ObstacleProcessor();
    ~ObstacleProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OBSTACLE"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Patterns & Song Chain ─────────────────────────────────────────────────
    std::array<Pattern, NUM_PATTERNS>    patterns;            // A-H
    std::array<SongSlot, NUM_SONG_SLOTS> songChain;
    int                                  songChainLength = 1; // 1-16 active slots
    bool                                 songLoopMode    = true;

    std::atomic<int>  editPatternIdx  { 0 };  // pattern shown in editor
    std::atomic<int>  playPatternIdx  { 0 };  // pattern currently playing
    std::atomic<int>  playSongSlot    { 0 };  // current slot in chain
    std::atomic<bool> nextRequested   { false }; // force-advance on next step 0

    // ── Sequencer state ───────────────────────────────────────────────────────
    std::atomic<int>   currentStep { -1 };
    std::atomic<float> bpm { 128.f };
    std::atomic<bool>  playing { false };

    // Randomize the current edit pattern (called from editor Rand button)
    void randomizePattern();

    // ── Parameters ────────────────────────────────────────────────────────────
    juce::AudioParameterFloat* bpmParam    = nullptr;

    // Per-track
    juce::AudioParameterFloat* trackVolParam  [NUM_TRACKS] = {};
    juce::AudioParameterBool*  trackMuteParam [NUM_TRACKS] = {};
    juce::AudioParameterFloat* trackDecParam  [NUM_TRACKS] = {}; // decay / env / filter

    // Global mix + FX
    juce::AudioParameterFloat* masterVolParam = nullptr;
    juce::AudioParameterFloat* reverbParam    = nullptr;
    juce::AudioParameterFloat* delayMixParam  = nullptr;
    juce::AudioParameterFloat* delayFeedParam = nullptr;
    juce::AudioParameterFloat* filterCutParam = nullptr;
    juce::AudioParameterFloat* swingParam     = nullptr;
    juce::AudioParameterFloat* driveParam     = nullptr;
    juce::AudioParameterInt*   keyParam       = nullptr; // semitone transpose -12..12

private:
    float sr = 44100.f;

    KickVoice  kick;
    SnareVoice snare;
    HihatVoice hihat;
    BassVoice  bass;
    LeadVoice  lead;
    PadVoice   pad;

    FXChain fx;

    double samplesPerStep = 0.0;
    double sampleCounter  = 0.0;
    int    seqStep        = 0;

    // Song chain tracking (audio thread only)
    int  loopCount = 0;

    juce::Random rng;

    bool wasHostPlaying      = false;
    bool wasPreviouslyPlaying = false;

    int midiActiveNote[NUM_TRACKS]; // -1 = no active note

    void buildDefaultPattern(int patIdx = 0);
    void triggerStep(int step, juce::MidiBuffer& midi, int samplePos, int patIdx);
    void nextSongSlot();
    void updateStepTiming();
    void sendAllNotesOff(juce::MidiBuffer& midi, int samplePos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObstacleProcessor)
};
