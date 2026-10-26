#pragma once
#include "PluginProcessor.h"
#include "NeonUI.h"

class TarareoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TarareoEditor (TarareoProcessor&);
    ~TarareoEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct DragZone : public juce::Component
    {
        TarareoProcessor& proc;
        explicit DragZone (TarareoProcessor& p) : proc (p)
        {
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (juce::Colour (0xff091522));
            g.fillRoundedRectangle (r, 11.0f);
            g.setColour (neon::cyan.withAlpha (0.65f));
            g.drawRoundedRectangle (r.reduced (0.5f), 11.0f, 1.6f);
            g.setFont (neon::font (13.0f, juce::Font::bold));
            g.setColour (neon::white);
            const auto f = proc.getLastFile();
            const auto mode = proc.getAudioMode();
            g.drawText (f.existsAsFile()
                           ? (mode == "singing" ? "↗ ARRASTRAR VOZ IA A FL STUDIO"
                                                : "↗ ARRASTRAR TARAREO GUÍA (NO ES VOZ IA)")
                           : "AQUÍ APARECERÁ LA PISTA PARA FL STUDIO",
                        getLocalBounds(), juce::Justification::centred);
        }

        void mouseDrag (const juce::MouseEvent&) override
        {
            auto f = proc.getLastFile();
            if (f.existsAsFile())
                juce::DragAndDropContainer::performExternalDragDropOfFiles (
                    { f.getFullPathName() }, false);
        }
    };

    void timerCallback() override;
    juce::Rectangle<int> scaleBounds (float x, float y, float w, float h) const;
    void paintLabel (juce::Graphics&, const juce::String&, float x, float y,
                     float w, juce::Colour = neon::muted);

    TarareoProcessor& proc;
    neon::NeonLookAndFeel laf; // Declarar antes de los componentes asociados
    neon::SingerView singer;

    juce::ComboBox genreBox, sectionBox, voiceBox, rangeBox, bpmModeBox, zoomBox;
    juce::Slider bpmSlider, creativitySlider;
    juce::TextEditor themeBox, lyricsBox;
    juce::Label status;
    juce::TextButton recBtn { "● GRABAR" }, genBtn { "✦ COMPONER" };
    DragZone dragZone { proc };

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ComboAttachment> genreAttach, sectionAttach, voiceAttach,
                                     rangeAttach, bpmModeAttach;
    std::unique_ptr<SliderAttachment> bpmAttach, creativityAttach;

    juce::String lastPath, lastAudioMode;
    int voiceIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TarareoEditor)
};
