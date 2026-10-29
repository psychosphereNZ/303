#include "PluginEditor.h"

namespace
{
    const juce::Colour kInk    (0xff1b1b1d);
    const juce::Colour kLight  (0xffe8e8e8);
    const juce::Colour kOrange (0xffff5a36);
    const juce::Colour kAmber  (0xffffb020);
    const juce::Colour kRed    (0xffff3b30);
    const juce::Colour kGreen  (0xff35d08a);
    const juce::Colour kBlue   (0xff3ec6ff);
    const juce::Colour kHot    (0x66ff7a1a);
    const int kLabelCol = 60;

    const char* kKeyNames[13] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C" };
    bool isSharp (int k) { return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }
}

Acid303Editor::Acid303Editor (Acid303Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    auto addKnob = [this] (std::vector<std::unique_ptr<Knob>>& list, const juce::String& id, const juce::String& text)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k->label.setText (text, juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);
        if (id != "root" && id != "length")   // Ganzzahl-Parameter behalten ihre Anzeige
        {
            k->slider.textFromValueFunction = [] (double v)
            {
                return juce::String (v, (v >= 10.0 || v <= -10.0) ? 1 : 2);
            };
            k->slider.updateText();
        }
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
        b.setColour (juce::TextButton::buttonOnColourId, on);
        addAndMakeVisible (b);
    };
    setupToggle (runBtn, kOrange);
    setupToggle (syncBtn, kBlue);
    runAtt  = std::make_unique<ButtonAtt> (proc.apvts, "run", runBtn);
    syncAtt = std::make_unique<ButtonAtt> (proc.apvts, "sync", syncBtn);

    addAndMakeVisible (randBtn);
    addAndMakeVisible (clearBtn);
    randBtn.onClick  = [this] { randomizePattern(); };
    clearBtn.onClick = [this] { clearPattern(); };

    waveBox.addItemList ({ "Saw", "Square" }, 1);
    addAndMakeVisible (waveBox);
    waveAtt = std::make_unique<ComboAtt> (proc.apvts, "wave", waveBox);

    filterBox.addItemList ({ "Filter 12 dB", "Filter 24 dB" }, 1);
    addAndMakeVisible (filterBox);
    filterAtt = std::make_unique<ComboAtt> (proc.apvts, "filter", filterBox);

    // ---- Write-Panel
    setupToggle (writeBtn, kBlue);
    writeBtn.onClick = [this] { writeMode = writeBtn.getToggleState(); repaint(); };

    setupToggle (downBtn, kAmber);
    setupToggle (upBtn, kAmber);
    setupToggle (accFlag, kRed);
    setupToggle (sldFlag, kGreen);
    downBtn.onClick = [this] { if (downBtn.getToggleState()) upBtn.setToggleState (false, juce::dontSendNotification); };
    upBtn.onClick   = [this] { if (upBtn.getToggleState()) downBtn.setToggleState (false, juce::dontSendNotification); };

    for (auto* b : { &restBtn, &tieBtn, &backBtn, &nextBtn })
        addAndMakeVisible (*b);
    restBtn.onClick = [this] { writeRest(); };
    tieBtn.onClick  = [this] { writeTie(); };
    backBtn.onClick = [this] { advanceCursor (-1); };
    nextBtn.onClick = [this] { advanceCursor (+1); };

    for (int k = 0; k < 13; ++k)
    {
        auto& b = keys[(size_t) k];
        b.setButtonText (kKeyNames[k]);
        const bool sharp = isSharp (k);
        b.setColour (juce::TextButton::buttonColourId, sharp ? juce::Colour (0xff2a2a2e) : juce::Colour (0xffe9e9ec));
        b.setColour (juce::TextButton::textColourOffId, sharp ? kLight : kInk);
        b.onClick = [this, k] { writeKey (k); };
        addAndMakeVisible (b);
    }

    // ---- Step-Raster
    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto s = std::make_unique<Step>();

        s->note.setSliderStyle (juce::Slider::LinearVertical);
        s->note.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 16);

        auto prep = [this] (juce::TextButton& b, juce::Colour on)
        {
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonOnColourId, on);
            addAndMakeVisible (b);
        };
        prep (s->gate,  kAmber);
        prep (s->tie,   kBlue);
        prep (s->acc,   kRed);
        prep (s->slide, kGreen);
        addAndMakeVisible (s->note);

        s->noteAtt  = std::make_unique<SliderAtt> (proc.apvts, "note" + n,  s->note);
        s->gateAtt  = std::make_unique<ButtonAtt> (proc.apvts, "gate" + n,  s->gate);
        s->tieAtt   = std::make_unique<ButtonAtt> (proc.apvts, "tie" + n,   s->tie);
        s->accAtt   = std::make_unique<ButtonAtt> (proc.apvts, "acc" + n,   s->acc);
        s->slideAtt = std::make_unique<ButtonAtt> (proc.apvts, "slide" + n, s->slide);
        steps.push_back (std::move (s));
    }

    setSize (940, 690);   // erst nach dem Anlegen aller Komponenten, damit resized() sie positioniert
    startTimerHz (30);
}

Acid303Editor::~Acid303Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Acid303Editor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

int Acid303Editor::patternLength() const
{
    return juce::jlimit (1, Acid303Processor::kSteps,
                         (int) std::lround (proc.apvts.getRawParameterValue ("length")->load()));
}

// ---------------------------------------------------------------- Write-Modus

void Acid303Editor::clearFlags()
{
    for (auto* b : { &downBtn, &upBtn, &accFlag, &sldFlag })
        b->setToggleState (false, juce::dontSendNotification);
}

void Acid303Editor::advanceCursor (int delta)
{
    const int len = patternLength();
    cursor = ((cursor + delta) % len + len) % len;
    repaint();
}

void Acid303Editor::writeKey (int key)
{
    // Down = tiefere, normal = mittlere, Up = höhere Oktave (Bereich 0..36 Halbtöne über ROOT)
    const int shift = downBtn.getToggleState() ? 0 : (upBtn.getToggleState() ? 24 : 12);
    const int note = juce::jlimit (0, 36, key + shift);

    // Vorhören
    const int root = (int) std::lround (proc.apvts.getRawParameterValue ("root")->load());
    proc.auditionRequest.store (juce::jlimit (0, 127, root + note));

    if (! writeMode)
        return;

    const auto n = juce::String (cursor + 1);
    setParam ("note" + n,  (float) note);
    setParam ("gate" + n,  1.0f);
    setParam ("tie" + n,   0.0f);
    setParam ("acc" + n,   accFlag.getToggleState() ? 1.0f : 0.0f);
    setParam ("slide" + n, sldFlag.getToggleState() ? 1.0f : 0.0f);
    clearFlags();
    advanceCursor (+1);
}

void Acid303Editor::writeRest()
{
    if (! writeMode) return;
    const auto n = juce::String (cursor + 1);
    setParam ("gate" + n,  0.0f);
    setParam ("tie" + n,   0.0f);
    setParam ("acc" + n,   0.0f);
    setParam ("slide" + n, 0.0f);
    clearFlags();
    advanceCursor (+1);
}

void Acid303Editor::writeTie()
{
    if (! writeMode) return;
    const auto n = juce::String (cursor + 1);
    setParam ("gate" + n,  1.0f);
    setParam ("tie" + n,   1.0f);
    setParam ("acc" + n,   0.0f);
    setParam ("slide" + n, sldFlag.getToggleState() ? 1.0f : 0.0f);
    clearFlags();
    advanceCursor (+1);
}

void Acid303Editor::randomizePattern()
{
    juce::Random rng;
    static const int scale[] = { 0, 0, 0, 3, 5, 7, 10, 12, 12, 15, 19, 24 };   // Moll-Pentatonik, Grundton betont

    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        const bool gateOn = rng.nextFloat() < 0.78f;
        setParam ("note" + n,  (float) (12 + scale[rng.nextInt (12)]));
        setParam ("gate" + n,  gateOn ? 1.0f : 0.0f);
        setParam ("tie" + n,   (gateOn && i > 0 && rng.nextFloat() < 0.12f) ? 1.0f : 0.0f);
        setParam ("acc" + n,   rng.nextFloat() < 0.25f ? 1.0f : 0.0f);
        setParam ("slide" + n, rng.nextFloat() < 0.20f ? 1.0f : 0.0f);
    }
}

void Acid303Editor::clearPattern()
{
    for (int i = 0; i < Acid303Processor::kSteps; ++i)
    {
        const auto n = juce::String (i + 1);
        setParam ("note" + n, 12.0f);
        setParam ("gate" + n, 0.0f);
        setParam ("tie" + n, 0.0f);
        setParam ("acc" + n, 0.0f);
        setParam ("slide" + n, 0.0f);
    }
    cursor = 0;
    repaint();
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
    // Silbernes Gehäuse
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd6d9de), 0.0f, 0.0f,
                                             juce::Colour (0xff9da1a8), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Gebürstete Struktur
    g.setColour (juce::Colour (0x0c000000));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    // Titel
    g.setColour (kInk);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("ACID303", 14, 8, 200, 28, juce::Justification::left);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("BASS LINE  /  16 STEP SEQUENCER", 150, 14, 300, 18, juce::Justification::left);
    g.setColour (kOrange);
    g.fillRect (14, 38, getWidth() - 28, 2);

    // Vertiefter Step-Bereich
    g.setColour (juce::Colour (0x22000000));
    g.fillRoundedRectangle (10.0f, (float) stepTop - 6.0f, (float) getWidth() - 20.0f,
                            (float) (stepBottom - stepTop) + 12.0f, 6.0f);

    if (steps.empty()) return;

    const int len = patternLength();

    // Spaltenmarkierung: Wiedergabe-Step (orange) bzw. Schreib-Cursor (blau)
    auto highlight = [&] (int idx, juce::Colour c)
    {
        auto& s = *steps[(size_t) idx];
        g.setColour (c);
        g.fillRoundedRectangle ((float) s.note.getX() - 2.0f, (float) stepTop,
                                (float) s.note.getWidth() + 4.0f, (float) (stepBottom - stepTop), 4.0f);
    };
    if (shownStep >= 0 && shownStep < (int) steps.size()) highlight (shownStep, kHot);
    if (writeMode && cursor < (int) steps.size())         highlight (cursor, kBlue.withAlpha (0.28f));

    // Steps hinter der Pattern-Länge abdunkeln
    for (int i = len; i < (int) steps.size(); ++i)
    {
        auto& s = *steps[(size_t) i];
        g.setColour (juce::Colour (0x55b8bcc2));
        g.fillRect (s.note.getX() - 3, stepTop, s.note.getWidth() + 6, stepBottom - stepTop);
    }

    // Lauflicht: eine LED pro Step
    for (int i = 0; i < (int) steps.size(); ++i)
    {
        auto& s = *steps[(size_t) i];
        const float cx = (float) s.note.getX() + (float) s.note.getWidth() * 0.5f;
        const float cy = (float) stepTop + 7.0f;

        const bool cursorLed = writeMode && i == cursor;
        const bool playLed   = ! writeMode && i == shownStep;
        const bool lit = cursorLed || playLed;
        const juce::Colour c = cursorLed ? kBlue : kRed;

        if (lit)
        {
            g.setColour (c.withAlpha (0.35f));
            g.fillEllipse (cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
        }
        g.setColour (lit ? c : juce::Colour (0xff5a2a27));
        g.fillEllipse (cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colour (0xff101012));
        g.drawEllipse (cx - 4.5f, cy - 4.5f, 9.0f, 9.0f, 1.0f);
    }

    g.setColour (kInk);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    for (size_t i = 0; i < steps.size(); ++i)
        g.drawText (juce::String ((int) i + 1), steps[i]->note.getX(), stepTop + 14, steps[i]->note.getWidth(), 16,
                    juce::Justification::centred);

    auto label = [&] (const juce::String& t, int y, int h)
    {
        g.drawText (t, 18, y, kLabelCol - 8, h, juce::Justification::centredLeft);
    };
    label ("NOTE",   steps[0]->note.getY(),  steps[0]->note.getHeight());
    label ("GATE",   steps[0]->gate.getY(),  steps[0]->gate.getHeight());
    label ("TIE",    steps[0]->tie.getY(),   steps[0]->tie.getHeight());
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
    auto l3 = left.removeFromTop (30);
    waveBox.setBounds   (l3.removeFromLeft (110).reduced (2));
    filterBox.setBounds (l3.removeFromLeft (140).reduced (2));
    layoutKnobs (rowB, seqKnobs);

    // Write-Panel: Zeile 1 = Modus + Klaviatur + Oktave, Zeile 2 = Flags + Rest/Tie + Cursor
    r.removeFromTop (8);
    auto w1 = r.removeFromTop (38);
    writeBtn.setBounds (w1.removeFromLeft (90).reduced (2));
    for (int k = 0; k < 13; ++k)
    {
        auto cell = w1.removeFromLeft (44).reduced (1, 0);
        keys[(size_t) k].setBounds (isSharp (k) ? cell.withTrimmedBottom (14) : cell);
    }
    w1.removeFromLeft (8);
    downBtn.setBounds (w1.removeFromLeft (60).reduced (2));
    upBtn.setBounds   (w1.removeFromLeft (60).reduced (2));

    r.removeFromTop (4);
    auto w2 = r.removeFromTop (30);
    accFlag.setBounds (w2.removeFromLeft (90).reduced (2));
    sldFlag.setBounds (w2.removeFromLeft (90).reduced (2));
    w2.removeFromLeft (10);
    restBtn.setBounds (w2.removeFromLeft (90).reduced (2));
    tieBtn.setBounds  (w2.removeFromLeft (90).reduced (2));
    w2.removeFromLeft (10);
    backBtn.setBounds (w2.removeFromLeft (90).reduced (2));
    nextBtn.setBounds (w2.removeFromLeft (90).reduced (2));

    r.removeFromTop (10);
    stepTop = r.getY();
    stepBottom = r.getBottom();

    auto grid = r;
    grid.removeFromLeft (kLabelCol);
    const int colW = grid.getWidth() / Acid303Processor::kSteps;

    for (auto& s : steps)
    {
        auto col = grid.removeFromLeft (colW).reduced (3, 0);
        col.removeFromTop (14);    // Lauflicht-LED
        col.removeFromTop (16);    // Step-Nummer
        s->note.setBounds  (col.removeFromTop (170));
        col.removeFromTop (4);
        s->gate.setBounds  (col.removeFromTop (28));
        col.removeFromTop (4);
        s->tie.setBounds   (col.removeFromTop (28));
        col.removeFromTop (4);
        s->acc.setBounds   (col.removeFromTop (28));
        col.removeFromTop (4);
        s->slide.setBounds (col.removeFromTop (28));
    }
}
