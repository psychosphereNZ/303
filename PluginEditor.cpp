#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg     (0xff17171a);
    const juce::Colour kPanel  (0xff26262b);
    const juce::Colour kText   (0xffe6e6e6);
    const juce::Colour kOrange (0xffff5a36);
    const juce::Colour kBlue   (0xff3ec6ff);
    const juce::Colour kGate   (0xffe0e0e0);
    const juce::Colour kHot    (0xff3a2a24);
    const int kLabelCol = 60;
}

Acid303Editor::Acid303Editor (Acid303Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (940, 580);

    auto addKnob = [this] (std::vector<std::unique_ptr<Knob>>& list, const juce::String& id, const juce::String& text)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, kOrange);
        k->slider.setColour (juce::Slider::thumbColourId, kText);
        k->slider.setColour (juce::Slider::textBoxTextColourId, kText);
        k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k->label.setText (text, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setColour (juce::Label::textColourId, kText);
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);
        list.push_back (std::move (k));
    };

    addKnob (soundKnobs, "cutoff", "CUTOFF");
    addKnob (soundKnobs, "reso",   "RESO");
    addKnob (soundKnobs, "envmod", "ENV MOD");
    addKnob (soundKnobs, "decay",  "DECAY");
    addKnob (soundKnobs, "accent", "ACCENT");
    addKnob (soundKnobs, "slide",  "SLIDE");
    addKnob (soundKnobs, "tune",   "TUNE");
    addKnob (soundKnobs, "volume", "VOLUME");

    addKnob (seqKnobs, "bpm",     "TEMPO");
    addKnob (seqKnobs, "root",    "ROOT");
    addKnob (seqKnobs, "length",  "LENGTH");
    addKnob (seqKnobs, "gatelen", "GATE LEN");

    auto setupToggle = [this] (juce::TextButton& b, juce::Colour on)
    {
        b.setClickingTogglesState (true);
        b.setColour (juce::TextButton::buttonColourId, kPanel);
        b.setColour (juce::TextButton::buttonOnColourId, on);
        b.setColour (juce::TextButton::textColourOffId, kText);
        b.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        addAndMakeVisible (b);
    };
    setupToggle (runBtn, kOrange);
    setupToggle (syncBtn, kBlue);
    runAtt  = std::make_unique<ButtonAtt> (proc.apvts, "run", runBtn);
    syncAtt = std::make_unique<ButtonAtt> (proc.apvts, "sync", syncBtn);

    for (auto* b : { &randBtn, &clearBtn })
    {
        b->setColour (juce::TextButton::buttonColourId, kPanel);
        b->setColour (juce::TextButton::textColourOffId, kText);
        addAndMakeVisible (*b);
    }
    randBtn.onClick  = [this] { randomizePattern(); };
    clearBtn.onClick = [this] { clearPattern(); };

    waveBox.addItemList ({ "Saw", "Square" }, 1);
    addAndMakeVisible (waveBox);
    waveAtt = std::make_unique<ComboAtt> (proc.apvts, "wave", waveBox);

    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto s = std::make_unique<Step>();

        s->note.setSliderStyle (juce::Slider::LinearVertical);
        s->note.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 16);
        s->note.setColour (juce::Slider::trackColourId, kOrange);
        s->note.setColour (juce::Slider::thumbColourId, kText);
        s->note.setColour (juce::Slider::textBoxTextColourId, kText);
        s->note.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

        auto prep = [this] (juce::TextButton& b, juce::Colour on)
        {
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId, kPanel);
            b.setColour (juce::TextButton::buttonOnColourId, on);
            addAndMakeVisible (b);
        };
        prep (s->gate,  kGate);
        prep (s->acc,   kOrange);
        prep (s->slide, kBlue);
        addAndMakeVisible (s->note);

        s->noteAtt  = std::make_unique<SliderAtt> (proc.apvts, "note" + n,  s->note);
        s->gateAtt  = std::make_unique<ButtonAtt> (proc.apvts, "gate" + n,  s->gate);
        s->accAtt   = std::make_unique<ButtonAtt> (proc.apvts, "acc" + n,   s->acc);
        s->slideAtt = std::make_unique<ButtonAtt> (proc.apvts, "slide" + n, s->slide);
        steps.push_back (std::move (s));
    }

    startTimerHz (30);
}

Acid303Editor::~Acid303Editor() { stopTimer(); }

void Acid303Editor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void Acid303Editor::randomizePattern()
{
    juce::Random rng;
    static const int scale[] = { 0, 0, 0, 3, 5, 7, 10, 12, 12, 15, 19, 24 };   // Moll-Pentatonik, Grundton betont

    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        setParam ("note" + n,  (float) scale[rng.nextInt (12)]);
        setParam ("gate" + n,  rng.nextFloat() < 0.78f ? 1.0f : 0.0f);
        setParam ("acc" + n,   rng.nextFloat() < 0.25f ? 1.0f : 0.0f);
        setParam ("slide" + n, rng.nextFloat() < 0.20f ? 1.0f : 0.0f);
    }
}

void Acid303Editor::clearPattern()
{
    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        setParam ("note" + n, 0.0f);
        setParam ("gate" + n, 0.0f);
        setParam ("acc" + n, 0.0f);
        setParam ("slide" + n, 0.0f);
    }
}

void Acid303Editor::timerCallback()
{
    const int c = proc.currentStep.load();
    if (c != shownStep)
    {
        shownStep = c;
        repaint();
    }
}

void Acid303Editor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kOrange);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("ACID303", 14, 8, 300, 28, juce::Justification::left);

    if (shownStep >= 0 && shownStep < (int) steps.size())
    {
        auto& s = *steps[(size_t) shownStep];
        g.setColour (kHot);
        g.fillRoundedRectangle ((float) s.note.getX() - 2.0f, (float) stepTop,
                                (float) s.note.getWidth() + 4.0f, (float) (stepBottom - stepTop), 4.0f);
    }

    g.setColour (kText);
    g.setFont (juce::FontOptions (11.0f));
    for (size_t i = 0; i < steps.size(); ++i)
        g.drawText (juce::String ((int) i + 1), steps[i]->note.getX(), stepTop, steps[i]->note.getWidth(), 16,
                    juce::Justification::centred);

    auto label = [&] (const juce::String& t, int y, int h)
    {
        g.drawText (t, 14, y, kLabelCol - 6, h, juce::Justification::centredLeft);
    };
    label ("NOTE",   steps[0]->note.getY(),  steps[0]->note.getHeight());
    label ("GATE",   steps[0]->gate.getY(),  steps[0]->gate.getHeight());
    label ("ACCENT", steps[0]->acc.getY(),   steps[0]->acc.getHeight());
    label ("SLIDE",  steps[0]->slide.getY(), steps[0]->slide.getHeight());
}

void Acid303Editor::resized()
{
    auto r = getLocalBounds().reduced (14);
    r.removeFromTop (34);

    auto layoutKnobs = [] (juce::Rectangle<int> area, std::vector<std::unique_ptr<Knob>>& list)
    {
        if (list.empty()) return;
        const int w = area.getWidth() / (int) list.size();
        for (auto& k : list)
        {
            auto cell = area.removeFromLeft (w);
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell);
        }
    };

    layoutKnobs (r.removeFromTop (96), soundKnobs);
    r.removeFromTop (6);

    auto rowB = r.removeFromTop (96);
    auto left = rowB.removeFromLeft (330);
    auto l1 = left.removeFromTop (32);
    runBtn.setBounds  (l1.removeFromLeft (100).reduced (2));
    syncBtn.setBounds (l1.removeFromLeft (110).reduced (2));
    auto l2 = left.removeFromTop (32);
    randBtn.setBounds  (l2.removeFromLeft (100).reduced (2));
    clearBtn.setBounds (l2.removeFromLeft (110).reduced (2));
    waveBox.setBounds (left.removeFromTop (30).removeFromLeft (210).reduced (2));
    layoutKnobs (rowB, seqKnobs);

    r.removeFromTop (10);
    stepTop = r.getY();
    stepBottom = r.getBottom();

    auto grid = r;
    grid.removeFromLeft (kLabelCol);
    const int colW = grid.getWidth() / Acid303Processor::kSteps;

    for (auto& s : steps)
    {
        auto col = grid.removeFromLeft (colW).reduced (3, 0);
        col.removeFromTop (16);
        s->note.setBounds  (col.removeFromTop (170));
        col.removeFromTop (4);
        s->gate.setBounds  (col.removeFromTop (28));
        col.removeFromTop (4);
        s->acc.setBounds   (col.removeFromTop (28));
        col.removeFromTop (4);
        s->slide.setBounds (col.removeFromTop (28));
    }
}
