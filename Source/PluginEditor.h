#pragma once
#include "PluginProcessor.h"
#include "AnalogUI.h"

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
        explicit DragZone (TarareoProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced (2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 6.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3e6c4), b.getX(), b.getY(),
                                                     juce::Colour (0xffd9c48c), b.getX(), b.getBottom(), false));
            g.fillRoundedRectangle (b, 6.0f);

            juce::Path outline, dashed;
            outline.addRoundedRectangle (b.reduced (4.0f), 4.0f);
            const float dashes[] = { 6.0f, 4.0f };
            juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dashes, 2);
            g.setColour (analog::ink.withAlpha (0.55f));
            g.fillPath (dashed);

            g.setColour (analog::ink);
            g.setFont (analog::serif (14.0f));
            g.drawText (proc.getLastFile().existsAsFile() ? "ARRASTRA ESTE AUDIO A FL STUDIO"
                                                          : "AQUI APARECERA TU VOZ CANTANDO",
                        getLocalBounds(), juce::Justification::centred);
        }

        void mouseDrag (const juce::MouseEvent&) override
        {
            auto f = proc.getLastFile();
            if (f.existsAsFile())
                juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false);
        }
    };

    void timerCallback() override;
    void paintBackdrop (juce::Graphics&, int w, int h);

    TarareoProcessor& proc;

    analog::AnalogLookAndFeel laf;      // debe declararse antes que los componentes que lo usan
    analog::SingerView singer;
    analog::VuMeter vu;
    juce::Slider genreKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> genreAttach;
    juce::Label genreLcd, status;
    juce::TextButton recBtn { "GRABAR" }, genBtn { "GENERAR" };
    juce::TextEditor lyricsBox;
    DragZone dragZone { proc };

    juce::Image backdrop;
    juce::Rectangle<int> knobArea;
    int genreIndex = -1;
    juce::String lastPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TarareoEditor)
};
