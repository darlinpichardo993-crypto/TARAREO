#pragma once
#include "PluginProcessor.h"

class TarareoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TarareoEditor (TarareoProcessor&);
    ~TarareoEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct DragZone : public juce::Component
    {
        TarareoProcessor& proc;
        explicit DragZone (TarareoProcessor& p) : proc (p) {}
        void paint (juce::Graphics& g) override
        {
            g.setColour (juce::Colour (0x33ffffff));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 12.0f);
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
            g.drawText (proc.getLastFile().existsAsFile() ? "Arrastra este audio a FL Studio"
                                                          : "Aqui aparecera tu voz cantando",
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

    TarareoProcessor& proc;
    juce::ComboBox genreBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> genreAttach;
    juce::TextButton recBtn { "GRABAR" }, genBtn { "GENERAR" };
    juce::Label status;
    juce::TextEditor lyricsBox;
    DragZone dragZone { proc };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TarareoEditor)
};
