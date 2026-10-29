#pragma once
#include "PluginProcessor.h"
#include "LookAndFeel303.h"
#include <array>
#include <memory>
#include <vector>

class Acid303Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Acid303Editor (Acid303Processor&);
    ~Acid303Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void randomizePattern();
    void clearPattern();
    void setParam (const juce::String& id, float plainValue);

    // Write-Modus (Pattern-Programmierung wie bei der 303)
    int  patternLength() const;
    void writeKey (int key);
    void writeRest();
    void writeTie();
    void advanceCursor (int delta);
    void clearFlags();

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    struct Step
    {
        juce::Slider note;
        juce::TextButton gate, tie, acc, slide;
        std::unique_ptr<SliderAtt> noteAtt;
        std::unique_ptr<ButtonAtt> gateAtt, tieAtt, accAtt, slideAtt;
    };

    Acid303LookAndFeel laf;   // muss vor allen Komponenten deklariert sein
    Acid303Processor& proc;

    std::vector<std::unique_ptr<Knob>> soundKnobs, seqKnobs;
    std::vector<std::unique_ptr<Step>> steps;

    juce::TextButton runBtn { "RUN" }, syncBtn { "HOST SYNC" }, randBtn { "RANDOM" }, clearBtn { "CLEAR" };
    juce::ComboBox waveBox, filterBox;
    std::unique_ptr<ButtonAtt> runAtt, syncAtt;
    std::unique_ptr<ComboAtt> waveAtt, filterAtt;

    // Write-Panel
    juce::TextButton writeBtn { "WRITE" }, downBtn { "DOWN" }, upBtn { "UP" },
                     accFlag { "ACCENT" }, sldFlag { "SLIDE" },
                     restBtn { "REST" }, tieBtn { "TIE" }, backBtn { "BACK" }, nextBtn { "NEXT" };
    std::array<juce::TextButton, 13> keys;
    bool writeMode = false;
    int cursor = 0;

    int shownStep = -2;
    int stepTop = 0, stepBottom = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Acid303Editor)
};
