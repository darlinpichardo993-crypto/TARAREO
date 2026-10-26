#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>

// Componentes visuales de estilo analogico para TARAREO (todo dibujado por codigo).
namespace analog
{
    const juce::Colour gold  (0xffe8c76a);
    const juce::Colour cream (0xfff3e6c4);
    const juce::Colour amber (0xffffb000);
    const juce::Colour ink   (0xff2b1d10);
    const juce::Colour ledRed (0xffff2a2a);

    inline juce::Font serif  (float h, int style = juce::Font::bold) { return juce::Font (juce::FontOptions ("Georgia", h, style)); }
    inline juce::Font mono   (float h, int style = juce::Font::bold) { return juce::Font (juce::FontOptions ("Courier New", h, style)); }
    inline juce::Font script (float h)                               { return juce::Font (juce::FontOptions ("Segoe Script", h, juce::Font::plain)); }

    //==========================================================================
    struct AnalogLookAndFeel : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                               float startA, float endA, juce::Slider&) override
        {
            auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
            const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
            const auto c = b.getCentre();
            const float angle = startA + pos * (endA - startA);

            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (c.x - r + 2.0f, c.y - r + 4.0f, r * 2.0f, r * 2.0f);

            g.setColour (juce::Colour (0xff15161a));
            g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

            g.setColour (juce::Colour (0xff7b7e86));
            for (int i = 0; i < 48; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / 48.0f;
                g.drawLine (c.x + std::sin (a) * (r - 4.0f), c.y - std::cos (a) * (r - 4.0f),
                            c.x + std::sin (a) * r,          c.y - std::cos (a) * r, 1.2f);
            }

            const float rb = r - 6.0f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e2d8), c.x - rb * 0.5f, c.y - rb * 0.7f,
                                                     juce::Colour (0xff3b3a38), c.x + rb * 0.6f, c.y + rb, true));
            g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f, 1.0f);

            juce::Path pointer;
            pointer.addRoundedRectangle (-2.2f, -rb + 3.0f, 4.4f, rb * 0.55f, 1.8f);
            g.setColour (juce::Colour (0xffb3261e));
            g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (c.x - 4.0f, c.y - 4.0f, 8.0f, 8.0f);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                   bool highlighted, bool down) override
        {
            auto b = button.getLocalBounds().toFloat().reduced (2.0f);
            const bool toggle = button.getClickingTogglesState();
            const bool on = button.getToggleState();

            juce::Colour base = on ? juce::Colour (0xff8e1b12) : juce::Colour (0xffe9dcb8);
            if (down) base = base.darker (0.25f);
            else if (highlighted) base = base.brighter (0.08f);

            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (b.translated (0.0f, down ? 1.0f : 3.0f), 8.0f);

            g.setGradientFill (juce::ColourGradient (base.brighter (0.25f), b.getX(), b.getY(),
                                                     base.darker (0.35f), b.getX(), b.getBottom(), false));
            g.fillRoundedRectangle (b, 8.0f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawRoundedRectangle (b, 8.0f, 1.5f);
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawRoundedRectangle (b.reduced (2.5f), 6.0f, 1.0f);

            if (toggle)
            {
                const float lx = b.getX() + 24.0f, ly = b.getCentreY();
                if (on)
                {
                    g.setGradientFill (juce::ColourGradient (ledRed.withAlpha (0.55f), lx, ly,
                                                             juce::Colours::transparentBlack, lx + 14.0f, ly, true));
                    g.fillEllipse (lx - 14.0f, ly - 14.0f, 28.0f, 28.0f);
                }
                g.setColour (on ? ledRed : juce::Colour (0xff4a0f0f));
                g.fillEllipse (lx - 6.0f, ly - 6.0f, 12.0f, 12.0f);
                g.setColour (juce::Colours::black.withAlpha (0.7f));
                g.drawEllipse (lx - 6.0f, ly - 6.0f, 12.0f, 12.0f, 1.2f);
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            g.setFont (serif (17.0f));
            g.setColour (button.getToggleState() ? cream : ink);
            auto area = button.getLocalBounds();
            if (button.getClickingTogglesState()) area = area.withTrimmedLeft (30);
            g.drawText (button.getButtonText(), area, juce::Justification::centred);
        }
    };

    //==========================================================================
    struct VuMeter : public juce::Component
    {
        void setLevel (float peak)
        {
            const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
            const float target = juce::jlimit (0.0f, 1.0f, (db + 40.0f) / 43.0f);
            needle += (target - needle) * (target > needle ? 0.35f : 0.10f);
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced (2.0f);
            g.setColour (juce::Colour (0xff0e0e10));
            g.fillRoundedRectangle (b, 10.0f);

            auto face = b.reduced (6.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7ebc3), face.getCentreX(), face.getBottom(),
                                                     juce::Colour (0xffd2bd82), face.getCentreX(), face.getY(), true));
            g.fillRoundedRectangle (face, 6.0f);

            g.saveState();
            g.reduceClipRegion (face.toNearestInt());

            const float px = face.getCentreX(), py = face.getBottom() - 8.0f;
            const float R = face.getHeight() * 0.88f;
            const float range = 0.95f;

            for (int i = 0; i <= 28; ++i)
            {
                const float t = (float) i / 28.0f;
                const float a = -range + t * 2.0f * range;
                const bool major = (i % 4 == 0);
                const float len = major ? 13.0f : 7.0f;
                g.setColour (t > 0.9f ? juce::Colour (0xffb3261e) : ink);
                g.drawLine (px + std::sin (a) * (R * 0.80f), py - std::cos (a) * (R * 0.80f),
                            px + std::sin (a) * (R * 0.80f + len), py - std::cos (a) * (R * 0.80f + len), major ? 1.8f : 1.0f);
            }

            g.setFont (serif (10.0f));
            g.setColour (ink);
            const char* names[] = { "-20", "-10", "0" };
            const float marks[] = { 0.465f, 0.70f, 0.93f };
            for (int i = 0; i < 3; ++i)
            {
                const float a = -range + marks[i] * 2.0f * range;
                const float tx = px + std::sin (a) * (R * 0.66f), ty = py - std::cos (a) * (R * 0.66f);
                g.drawText (names[i], juce::Rectangle<float> (tx - 16.0f, ty - 7.0f, 32.0f, 14.0f), juce::Justification::centred);
            }

            g.setFont (serif (20.0f, juce::Font::bold | juce::Font::italic));
            g.drawText ("VU", juce::Rectangle<float> (px - 24.0f, py - 54.0f, 48.0f, 24.0f), juce::Justification::centred);
            g.setFont (serif (9.0f, juce::Font::plain));
            g.drawText ("ENTRADA", juce::Rectangle<float> (face.getX() + 8.0f, face.getY() + 4.0f, 70.0f, 12.0f), juce::Justification::centredLeft);

            const float a = -range + needle * 2.0f * range;
            const float tx = px + std::sin (a) * (R * 0.95f), ty = py - std::cos (a) * (R * 0.95f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawLine (px + 2.0f, py + 2.0f, tx + 2.0f, ty + 2.0f, 2.0f);
            g.setColour (juce::Colour (0xffb3261e));
            g.drawLine (px, py, tx, ty, 2.0f);
            g.setColour (juce::Colour (0xff1a1a1a));
            g.fillEllipse (px - 7.0f, py - 7.0f, 14.0f, 14.0f);
            g.restoreState();

            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), face.getX(), face.getY(),
                                                     juce::Colours::transparentWhite, face.getX(), face.getCentreY(), false));
            g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.5f), 6.0f);
        }

        float needle = 0.0f;
    };

    //==========================================================================
    struct SingerView : public juce::Component
    {
        SingerView() { buildSilhouette(); }

        void setLevel (float l)
        {
            level = level * 0.8f + juce::jlimit (0.0f, 1.0f, l * 3.0f) * 0.2f;
            hue = std::fmod (hue + 0.0015f, 1.0f);
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            const float cx = b.getCentreX();

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1233), cx, b.getY(),
                                                     juce::Colour (0xff07060f), cx, b.getBottom(), false));
            g.fillRoundedRectangle (b, 10.0f);

            const auto c1 = juce::Colour::fromHSV (hue, 0.85f, 1.0f, 1.0f);
            const auto c2 = juce::Colour::fromHSV (std::fmod (hue + 0.18f, 1.0f), 0.9f, 1.0f, 1.0f);

            juce::Path cone;
            cone.addTriangle (cx - 12.0f, b.getY() + 2.0f, cx + 12.0f, b.getY() + 2.0f, cx + b.getWidth() * 0.5f, b.getBottom());
            cone.addTriangle (cx - 12.0f, b.getY() + 2.0f, cx + 12.0f, b.getY() + 2.0f, cx - b.getWidth() * 0.5f, b.getBottom());
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), cx, b.getY(),
                                                     juce::Colours::transparentWhite, cx, b.getBottom(), false));
            g.fillPath (cone);

            const float cy = b.getCentreY() + 20.0f;
            g.setGradientFill (juce::ColourGradient (c1.withAlpha (0.20f + 0.35f * level), cx, cy,
                                                     juce::Colours::transparentBlack, cx, cy + b.getWidth() * 0.9f, true));
            g.fillEllipse (cx - b.getWidth() * 0.9f, cy - b.getWidth() * 0.9f, b.getWidth() * 1.8f, b.getWidth() * 1.8f);

            auto area = b.reduced (24.0f, 34.0f);
            const float s = juce::jmin (area.getWidth() / 100.0f, area.getHeight() / 140.0f);
            const auto t = juce::AffineTransform::scale (s).translated (area.getCentreX() - 50.0f * s, area.getBottom() - 140.0f * s);

            g.setGradientFill (juce::ColourGradient (c1.withAlpha (0.35f), cx, area.getBottom() - 6.0f,
                                                     juce::Colours::transparentBlack, cx + 60.0f, area.getBottom() + 14.0f, true));
            g.fillEllipse (cx - 70.0f, area.getBottom() - 8.0f, 140.0f, 24.0f);

            for (auto& part : parts)
            {
                if (part.accent) continue;
                g.setColour (c1.withAlpha (0.18f + 0.40f * level));
                g.strokePath (part.path, juce::PathStrokeType (7.0f), t);
            }

            const juce::ColourGradient grad (c1, cx, area.getY(), c2, cx, area.getBottom(), false);
            for (auto& part : parts)
            {
                if (part.accent) g.setColour (cream);
                else             g.setGradientFill (grad);
                g.fillPath (part.path, t);
            }

            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.5f);
        }

    private:
        struct Part { juce::Path path; bool accent = false; };

        void add (const juce::Path& p, bool accent = false) { parts.push_back ({ p, accent }); }

        void addStroke (const std::vector<juce::Point<float>>& pts, float thickness, bool accent = false)
        {
            juce::Path line, out;
            line.startNewSubPath (pts[0]);
            for (size_t i = 1; i < pts.size(); ++i) line.lineTo (pts[i]);
            juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (out, line);
            add (out, accent);
        }

        void buildSilhouette()
        {
            juce::Path p;
            p.addEllipse (40.0f, 10.0f, 20.0f, 24.0f);                       add (p);          // cabeza
            p.clear(); p.startNewSubPath (38, 16); p.lineTo (44, 5); p.lineTo (58, 4);
            p.lineTo (64, 12); p.lineTo (62, 18); p.lineTo (58, 12); p.lineTo (44, 14);
            p.closeSubPath();                                                add (p);          // pelo con tupe
            p.clear(); p.addRectangle (46.0f, 32.0f, 8.0f, 15.0f);          add (p);          // cuello
            p.clear(); p.startNewSubPath (30, 52); p.lineTo (37, 45); p.lineTo (63, 45);
            p.lineTo (70, 52); p.lineTo (62, 94); p.lineTo (38, 94);
            p.closeSubPath();                                                add (p);          // torso
            p.clear(); p.startNewSubPath (39, 92); p.lineTo (51, 92); p.lineTo (42, 138);
            p.lineTo (30, 138); p.closeSubPath();                            add (p);          // pierna izq
            p.clear(); p.startNewSubPath (49, 92); p.lineTo (61, 92); p.lineTo (72, 138);
            p.lineTo (60, 138); p.closeSubPath();                            add (p);          // pierna der
            p.clear(); p.addEllipse (25.0f, 134.0f, 20.0f, 6.0f);           add (p);          // zapatos
            p.clear(); p.addEllipse (58.0f, 134.0f, 22.0f, 6.0f);           add (p);
            addStroke ({ { 66, 52 }, { 84, 58 }, { 90, 36 } }, 8.0f);                        // brazo derecho arriba
            p.clear(); p.addEllipse (86.0f, 28.0f, 9.0f, 10.0f);            add (p);          // puno
            addStroke ({ { 34, 52 }, { 20, 70 }, { 41, 40 } }, 8.0f);                         // brazo con micro
            addStroke ({ { 41, 40 }, { 45, 33 } }, 4.0f, true);                               // mango del micro
            p.clear(); p.addEllipse (43.0f, 26.0f, 8.0f, 8.0f);             add (p, true);    // cabeza del micro
        }

        std::vector<Part> parts;
        float level = 0.0f, hue = 0.88f;
    };
}
