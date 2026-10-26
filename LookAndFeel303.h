#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// Skin im Stil der TB-303: silbernes Gehäuse, dunkle Drehknöpfe mit Skala, LED-artige Step-Tasten.
class Acid303LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Acid303LookAndFeel()
    {
        const auto ink = juce::Colour (0xff1b1b1d);
        const auto light = juce::Colour (0xffe8e8e8);
        const auto dark = juce::Colour (0xff2a2a2e);

        setColour (juce::Slider::textBoxTextColourId, ink);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, ink);

        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3a3f));
        setColour (juce::TextButton::textColourOffId, light);
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff111111));

        setColour (juce::ComboBox::backgroundColourId, dark);
        setColour (juce::ComboBox::textColourId, light);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff111111));
        setColour (juce::ComboBox::arrowColourId, light);
        setColour (juce::PopupMenu::backgroundColourId, dark);
        setColour (juce::PopupMenu::textColourId, light);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffff5a36));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.0f);
        const float size = juce::jmin (area.getWidth(), area.getHeight());
        const auto c = area.getCentre();
        const float outer = size * 0.5f;
        const float knobR = outer * 0.72f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        // Skala
        g.setColour (juce::Colour (0xff2a2a2e));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float s = std::sin (a), co = std::cos (a);
            const bool major = (i % 5 == 0);
            const float r1 = outer * 0.84f;
            const float r2 = outer * (major ? 1.0f : 0.94f);
            g.drawLine (c.x + s * r1, c.y - co * r1, c.x + s * r2, c.y - co * r2, major ? 1.6f : 1.0f);
        }

        // Schatten
        g.setColour (juce::Colour (0x40000000));
        g.fillEllipse (c.x - knobR + 1.5f, c.y - knobR + 3.0f, knobR * 2.0f, knobR * 2.0f);

        // Silberner Rand
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f0f0), c.x, c.y - knobR,
                                                 juce::Colour (0xff868990), c.x, c.y + knobR, false));
        g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

        // Dunkler Knopfkörper
        const float bodyR = knobR * 0.86f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff55555b), c.x - bodyR * 0.3f, c.y - bodyR * 0.4f,
                                                 juce::Colour (0xff131315), c.x + bodyR, c.y + bodyR, true));
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Zeiger
        const float s = std::sin (angle), co = std::cos (angle);
        g.setColour (juce::Colour (0xfff4f4f4));
        g.drawLine (c.x + s * bodyR * 0.25f, c.y - co * bodyR * 0.25f,
                    c.x + s * bodyR * 0.92f, c.y - co * bodyR * 0.92f, 2.5f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const float cx = (float) x + (float) width * 0.5f;
        const float top = (float) y + 4.0f, bottom = (float) (y + height) - 4.0f;

        // Rille
        g.setColour (juce::Colour (0xff1b1b1d));
        g.fillRoundedRectangle (cx - 3.0f, top, 6.0f, bottom - top, 3.0f);

        // Füllung bis zum Regler
        g.setColour (juce::Colour (0xffff5a36));
        g.fillRoundedRectangle (cx - 1.5f, sliderPos, 3.0f, juce::jmax (0.0f, bottom - sliderPos), 1.5f);

        // Metallkappe
        auto cap = juce::Rectangle<float> (cx - 12.0f, sliderPos - 7.0f, 24.0f, 14.0f);
        g.setColour (juce::Colour (0x50000000));
        g.fillRoundedRectangle (cap.translated (0.0f, 1.5f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2f2), 0.0f, cap.getY(),
                                                 juce::Colour (0xff8d9097), 0.0f, cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (juce::Colour (0xff1b1b1d));
        g.drawLine (cap.getX() + 3.0f, cap.getCentreY(), cap.getRight() - 3.0f, cap.getCentreY(), 1.5f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bg,
                               bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        auto base = bg;
        if (down)      base = base.darker (0.2f);
        else if (over) base = base.brighter (0.1f);

        g.setGradientFill (juce::ColourGradient (base.brighter (0.25f), 0.0f, r.getY(),
                                                 base.darker (0.25f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);

        if (b.getToggleState())
        {
            g.setColour (juce::Colours::white.withAlpha (0.45f));
            g.drawLine (r.getX() + 4.0f, r.getY() + 2.0f, r.getRight() - 4.0f, r.getY() + 2.0f, 1.0f);
        }

        g.setColour (juce::Colour (0xff101012));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
};
