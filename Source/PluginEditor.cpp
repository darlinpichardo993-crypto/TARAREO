#include "PluginEditor.h"

TarareoEditor::TarareoEditor (TarareoProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setSize (520, 460);

    genreBox.addItemList (juce::StringArray { "Bachata", "Merengue", "Reggaeton", "Trap", "R&B", "Pop" }, 1);
    addAndMakeVisible (genreBox);
    genreAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "genre", genreBox);

    recBtn.setClickingTogglesState (true);
    recBtn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe63946));
    recBtn.onClick = [this] { if (recBtn.getToggleState()) proc.startRecording(); else proc.stopRecording(); };
    genBtn.onClick = [this] { recBtn.setToggleState (false, juce::dontSendNotification); proc.generate(); };
    addAndMakeVisible (recBtn);
    addAndMakeVisible (genBtn);

    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (status);

    lyricsBox.setMultiLine (true);
    lyricsBox.setReadOnly (true);
    lyricsBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x33000000));
    lyricsBox.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    lyricsBox.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    lyricsBox.setFont (juce::FontOptions (16.0f));
    addAndMakeVisible (lyricsBox);
    addAndMakeVisible (dragZone);

    startTimerHz (5);
}

void TarareoEditor::timerCallback()
{
    status.setText (proc.getStatus(), juce::dontSendNotification);
    auto l = proc.getLyrics();
    if (l != lyricsBox.getText()) lyricsBox.setText (l, false);
    if (! proc.isRecording()) recBtn.setToggleState (false, juce::dontSendNotification);
    dragZone.repaint();
}

void TarareoEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0f0c29), 0, 0,
                                             juce::Colour (0xff302b63), 0, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colour (0xffff4ecd));
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
    g.drawText ("TARAREO", 0, 14, getWidth(), 50, juce::Justification::centred);

    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Tararea. La IA escribe y canta.", 0, 62, getWidth(), 20, juce::Justification::centred);

    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::italic));
    g.drawText ("Made By ORYMY And YMY Records", 0, getHeight() - 28, getWidth(), 20, juce::Justification::centred);
}

void TarareoEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (70);
    genreBox.setBounds (r.removeFromTop (32));
    r.removeFromTop (12);
    auto row = r.removeFromTop (44);
    recBtn.setBounds (row.removeFromLeft (row.getWidth() / 2 - 6));
    row.removeFromLeft (12);
    genBtn.setBounds (row);
    r.removeFromTop (10);
    status.setBounds (r.removeFromTop (24));
    r.removeFromTop (8);
    dragZone.setBounds (r.removeFromBottom (56));
    r.removeFromBottom (8);
    r.removeFromBottom (24);
    lyricsBox.setBounds (r);
}
