#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// TARAREO: illustraciones vectoriales code-native, sin imágenes externas.
namespace neon
{
    const juce::Colour bg (0xff090b15);
    const juce::Colour panel (0xff141828);
    const juce::Colour border (0xff30364e);
    const juce::Colour cyan (0xff55e9ff);
    const juce::Colour violet (0xffaa82ff);
    const juce::Colour pink (0xffff6dd1);
    const juce::Colour white (0xfff5f6ff);
    const juce::Colour muted (0xff9ca9c8);

    inline juce::Font font (float size, int style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions ("Segoe UI", size, style));
    }

    inline void panelBackground (juce::Graphics& g, juce::Rectangle<float> b,
                                 juce::Colour accent = violet)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (b.translated (0.0f, 5.0f), 18.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a2033), b.getX(), b.getY(),
                                                 panel, b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 18.0f);
        g.setColour (accent.withAlpha (0.32f));
        g.drawRoundedRectangle (b.reduced (0.6f), 18.0f, 1.4f);
    }

    struct NeonLookAndFeel : public juce::LookAndFeel_V4
    {
        NeonLookAndFeel()
        {
            setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0c1120));
            setColour (juce::ComboBox::outlineColourId, border);
            setColour (juce::ComboBox::textColourId, white);
            setColour (juce::ComboBox::arrowColourId, cyan);
            setColour (juce::PopupMenu::backgroundColourId, panel);
            setColour (juce::PopupMenu::textColourId, white);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff333759));
            setColour (juce::Slider::trackColourId, border);
            setColour (juce::Slider::backgroundColourId, border);
            setColour (juce::Slider::thumbColourId, cyan);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour&, bool hovered, bool down) override
        {
            const auto b = button.getLocalBounds().toFloat().reduced (2.0f);
            auto fill = button.getComponentID() == "generate" ? violet : juce::Colour (0xff203549);
            if (button.getToggleState()) fill = juce::Colour (0xff8d204b);
            if (down) fill = fill.darker (0.22f);
            if (hovered) fill = fill.brighter (0.14f);
            g.setColour (fill.withAlpha (button.isEnabled() ? 1.0f : 0.35f));
            g.fillRoundedRectangle (b, 10.0f);
            g.setColour ((button.getComponentID() == "generate" ? pink : cyan).withAlpha (0.65f));
            g.drawRoundedRectangle (b.reduced (0.6f), 10.0f, 1.4f);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            g.setFont (font (15.0f, juce::Font::bold));
            g.setColour (white.withAlpha (button.isEnabled() ? 1.0f : 0.5f));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
        }
    };

    struct SingerView : public juce::Component
    {
        void setFemale (bool next)
        {
            if (female != next) { female = next; repaint(); }
        }

        void setLevel (float next)
        {
            level = level * 0.82f + juce::jlimit (0.0f, 1.0f, next * 2.5f) * 0.18f;
            phase += 0.11f;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            panelBackground (g, b.reduced (1.0f), female ? pink : cyan);
            g.setColour (muted);
            g.setFont (font (12.0f, juce::Font::bold));
            g.drawText ("SINGING AVATAR", b.reduced (18.0f, 12.0f).removeFromTop (25.0f),
                        juce::Justification::centred);

            const float s = juce::jmin ((b.getWidth() - 28.0f) / 220.0f,
                                       (b.getHeight() - 100.0f) / 380.0f);
            const auto center = b.getCentre();
            juce::Graphics::ScopedSaveState guard (g);
            g.addTransform (juce::AffineTransform::scale (s)
                            .translated (center.x - 110.0f * s, center.y - 184.0f * s));

            const auto accent = female ? pink : cyan;
            const auto outline = accent.withAlpha (0.9f);
            // deliberately naive, hand-drawn outline; mouth animates to input level
            juce::Path torso;
            torso.startNewSubPath (61, 205);
            torso.cubicTo (22, 220, 33, 295, 43, 365);
            torso.lineTo (182, 365);
            torso.cubicTo (199, 294, 191, 222, 159, 205);
            torso.closeSubPath();
            g.setColour (juce::Colour (0xff242b43));
            g.fillPath (torso);
            g.setColour (outline);
            g.strokePath (torso, juce::PathStrokeType (3.0f));

            g.setColour (juce::Colour (0xffe0b08e));
            g.fillRoundedRectangle (99.0f, 187.0f, 22.0f, 40.0f, 7.0f);
            g.setColour (juce::Colour (0xffe8bc9f));
            g.fillEllipse (58.0f, 81.0f, 104.0f, 130.0f);
            g.setColour (juce::Colour (0xff111725));
            g.drawEllipse (58.0f, 81.0f, 104.0f, 130.0f, 3.1f);

            if (! female)
            {
                // ORYMY-inspired doodle: blonde buzz cut, moustache, chin goatee
                g.setColour (juce::Colour (0xffffe58a));
                juce::Path buzz;
                buzz.startNewSubPath (61, 118);
                buzz.cubicTo (55, 55, 169, 52, 160, 118);
                buzz.lineTo (151, 103);
                buzz.cubicTo (119, 86, 94, 88, 70, 105);
                buzz.closeSubPath();
                g.fillPath (buzz);
                g.setColour (juce::Colour (0xff3a2d21));
                g.drawLine (100, 164, 111, 167, 3.5f);
                g.drawLine (111, 167, 123, 164, 3.5f);
                juce::Path goatee;
                goatee.startNewSubPath (105, 187);
                goatee.lineTo (116, 187);
                goatee.lineTo (111, 205);
                goatee.closeSubPath();
                g.fillPath (goatee);
            }
            else
            {
                // Female doodle is illustrative and is not a real person's likeness.
                g.setColour (juce::Colour (0xff342444));
                juce::Path hair;
                hair.startNewSubPath (64, 141);
                hair.cubicTo (33, 78, 76, 48, 114, 52);
                hair.cubicTo (177, 49, 186, 97, 162, 145);
                hair.lineTo (178, 228);
                hair.lineTo (148, 213);
                hair.lineTo (149, 120);
                hair.cubicTo (128, 96, 91, 97, 71, 124);
                hair.lineTo (75, 213);
                hair.lineTo (47, 224);
                hair.closeSubPath();
                g.fillPath (hair);
                g.setColour (pink.withAlpha (0.85f));
                g.strokePath (hair, juce::PathStrokeType (2.0f));
            }

            g.setColour (juce::Colour (0xff1a1a23));
            g.fillEllipse (85.0f, 142.0f, 6.0f, 8.0f);
            g.fillEllipse (132.0f, 142.0f, 6.0f, 8.0f);
            const float mouthHeight = 6.0f + 26.0f * level;
            g.setColour (juce::Colour (0xff663548));
            g.fillEllipse (103.0f, 170.0f, 17.0f, mouthHeight);
            g.setColour (outline);
            g.drawEllipse (103.0f, 170.0f, 17.0f, mouthHeight, 1.8f);

            // The microphone is deliberately drawn with crooked lines.
            g.setColour (outline);
            g.drawLine (149, 211, 188, 170, 6.0f);
            g.setColour (juce::Colour (0xffdce6fa));
            g.fillRoundedRectangle (177, 150, 24, 39, 10);
            g.setColour (juce::Colour (0xff30364a));
            g.drawRoundedRectangle (177, 150, 24, 39, 10, 2.0f);
            g.setColour (accent.withAlpha (0.75f));
            for (int i = 0; i < 3; ++i)
            {
                const float wave = std::sin (phase + (float) i);
                g.drawEllipse (180.0f + i * 11.0f, 120.0f - i * 7.0f + wave * 3.0f,
                               7.0f, 7.0f, 1.6f);
            }
            g.setColour (white);
            g.setFont (font (15.0f, juce::Font::bold));
            g.drawText (female ? "VOZ FEMENINA" : "ORYMY / VOZ MASCULINA",
                        2, 373, 216, 28, juce::Justification::centred);
        }

    private:
        bool female = false;
        float level = 0.0f, phase = 0.0f;
    };
}
