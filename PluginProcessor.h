#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

// TB-303-artiger Acid-Bass-Synth mit 16-Step-Sequencer (Note, Gate, Tie, Accent, Slide)
// und wahlweise 12-dB- oder 24-dB-Hauptfilter.
class Acid303Processor : public juce::AudioProcessor
{
public:
    static constexpr int kSteps = 16;

    Acid303Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Acid303"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentStep { -1 };       // für die Lauflicht-Anzeige im Editor
    std::atomic<int> auditionRequest { -1 };   // MIDI-Note zum Vorhören (Write-Modus)

private:
    using P = std::atomic<float>*;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Synth
    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void renderChunk (float* out, int numSamples);
    static float polyBlep (float t, float dt);

    // Sequencer
    void triggerStep (int stepInt, int length, double inc);
    void stopSequencer();

    juce::dsp::LadderFilter<float> filter;
    int currentFilterMode = -1;
    double sr = 44100.0;

    // Synth-Zustand
    std::vector<int> heldNotes;
    double phase = 0.0;
    float freq = 110.0f, targetFreq = 110.0f;
    bool sliding = false, gate = false, accent = false;
    float filterEnv = 0.0f, ampEnv = 0.0f, accentEnv = 0.0f;

    // Sequencer-Zustand
    double stepPos = 0.0;        // Position in 16tel-Schritten
    int lastTriggered = -1;
    int seqNote = -1;
    int gateCountdown = -1;
    bool wasRunning = false;

    // Vorhören
    int auditionNote = -1;
    int auditionSamplesLeft = 0;

    // Gecachte Parameterzeiger
    P pWave {}, pFilter {}, pTune {}, pCutoff {}, pReso {}, pEnvMod {}, pDecay {}, pAccent {}, pSlide {}, pVolume {};
    P pRun {}, pSync {}, pBpm {}, pRoot {}, pLength {}, pGateLen {};
    P pNote[kSteps] {}, pGate[kSteps] {}, pTie[kSteps] {}, pAcc[kSteps] {}, pStepSlide[kSteps] {};
};
