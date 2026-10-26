#include "PluginEditor.h"

namespace
{
    const juce::StringArray& genreNames()
    {
        static const juce::StringArray names { "BACHATA", "MERENGUE", "REGGAETON", "TRAP", "R&B", "POP" };
        return names;
    }

    constexpr int kPanelX = 284, kPanelW = 448;
}

TarareoEditor::TarareoEditor (TarareoProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    genreKnob.setLookAndFeel (&laf);
    recBtn.setLookAndFeel (&laf);
    genBtn.setLookAndFeel (&laf);

    addAndMakeVisible (singer);
    addAndMakeVisible (vu);

    genreKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    genreKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    genreKnob.setRotaryParameters (-2.4f, 2.4f, true);
    genreKnob.setMouseDragSensitivity (150);
    addAndMakeVisible (genreKnob);
    genreAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "genre", genreKnob);

    genreLcd.setJustificationType (juce::Justification::centred);
    genreLcd.setFont (analog::mono (20.0f));
    genreLcd.setColour (juce::Label::textColourId, analog::amber);
    genreLcd.setColour (juce::Label::backgroundColourId, juce::Colour (0xff120c04));
    genreLcd.setColour (juce::Label::outlineColourId, juce::Colour (0xff6b5a3a));
    addAndMakeVisible (genreLcd);

    recBtn.setClickingTogglesState (true);
    recBtn.onClick = [this]
    {
        if (recBtn.getToggleState()) proc.startRecording(); else proc.stopRecording();
    };
    genBtn.onClick = [this]
    {
        recBtn.setToggleState (false, juce::dontSendNotification);
        proc.generate();
    };
    addAndMakeVisible (recBtn);
    addAndMakeVisible (genBtn);

    status.setJustificationType (juce::Justification::centred);
    status.setFont (analog::serif (14.0f, juce::Font::italic));
    status.setColour (juce::Label::textColourId, analog::cream);
    addAndMakeVisible (status);

    lyricsBox.setMultiLine (true);
    lyricsBox.setReadOnly (true);
    lyricsBox.setFont (analog::mono (16.0f, juce::Font::plain));
    lyricsBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xffefe3bf));
    lyricsBox.setColour (juce::TextEditor::textColourId, analog::ink);
    lyricsBox.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff6b5a3a));
    lyricsBox.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff6b5a3a));
    addAndMakeVisible (lyricsBox);

    addAndMakeVisible (dragZone);

    setSize (760, 560);
    startTimerHz (30);
}

TarareoEditor::~TarareoEditor()
{
    genreKnob.setLookAndFeel (nullptr);
    recBtn.setLookAndFeel (nullptr);
    genBtn.setLookAndFeel (nullptr);
}

void TarareoEditor::timerCallback()
{
    const float lvl = proc.getInputLevel();
    vu.setLevel (lvl);
    singer.setLevel (lvl);

    status.setText (proc.getStatus(), juce::dontSendNotification);

    const auto l = proc.getLyrics();
    if (l != lyricsBox.getText()) lyricsBox.setText (l, false);

    const bool rec = proc.isRecording();
    if (recBtn.getToggleState() != rec) recBtn.setToggleState (rec, juce::dontSendNotification);
    recBtn.setButtonText (rec ? "DETENER" : "GRABAR");

    const int idx = juce::jlimit (0, genreNames().size() - 1,
                                  (int) std::round (proc.apvts.getRawParameterValue ("genre")->load()));
    if (idx != genreIndex)
    {
        genreIndex = idx;
        genreLcd.setText (genreNames()[idx], juce::dontSendNotification);
        repaint (knobArea);
    }

    const auto path = proc.getLastFile().getFullPathName();
    if (path != lastPath) { lastPath = path; dragZone.repaint(); }
}

void TarareoEditor::paintBackdrop (juce::Graphics& g, int w, int h)
{
    const auto W = (float) w, H = (float) h;

    // madera
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a3720), 0.0f, 0.0f, juce::Colour (0xff2c180c), 0.0f, H, false));
    g.fillAll();
    juce::Random rnd (42);
    for (float y = 0.0f; y < H; y += 2.0f)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::black : juce::Colour (0xffb98a5a)).withAlpha (0.03f + rnd.nextFloat() * 0.07f));
        g.drawLine (0.0f, y, W, y + rnd.nextFloat() * 3.0f - 1.5f, 1.0f);
    }

    // placa metalica
    auto plate = juce::Rectangle<float> (12.0f, 12.0f, W - 24.0f, H - 24.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (plate.translated (0.0f, 4.0f), 14.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff30333b), 0.0f, plate.getY(), juce::Colour (0xff15161a), 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 14.0f);
    for (float y = plate.getY() + 2.0f; y < plate.getBottom() - 2.0f; y += 2.0f)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.02f + rnd.nextFloat() * 0.025f));
        g.drawHorizontalLine ((int) y, plate.getX() + 6.0f, plate.getRight() - 6.0f);
    }
    g.setColour (analog::gold.withAlpha (0.45f));
    g.drawRoundedRectangle (plate.reduced (5.0f), 10.0f, 1.3f);

    // tornillos
    const juce::Point<float> screws[] = { { 24, 24 }, { W - 24, 24 }, { 24, H - 24 }, { W - 24, H - 24 } };
    for (auto s : screws)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0d0), s.x - 2.0f, s.y - 2.0f, juce::Colour (0xff4a4a4a), s.x + 4.0f, s.y + 4.0f, true));
        g.fillEllipse (s.x - 4.5f, s.y - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawLine (s.x - 3.0f, s.y + 1.5f, s.x + 3.0f, s.y - 1.5f, 1.3f);
    }

    // titulo grabado
    const auto titleBox = juce::Rectangle<int> (kPanelX, 30, kPanelW, 58);
    g.setFont (analog::serif (50.0f, juce::Font::bold | juce::Font::italic));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawText ("TARAREO", titleBox.translated (2, 3), juce::Justification::centred);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7e1a0), 0.0f, 34.0f, juce::Colour (0xffb98a2e), 0.0f, 84.0f, false));
    g.drawText ("TARAREO", titleBox, juce::Justification::centred);

    g.setFont (analog::script (15.0f));
    g.setColour (analog::cream.withAlpha (0.85f));
    g.drawText ("Tararea. La IA escribe y canta.", juce::Rectangle<int> (kPanelX, 86, kPanelW, 26), juce::Justification::centred);

    g.setColour (analog::gold.withAlpha (0.45f));
    g.drawHorizontalLine (114, (float) kPanelX, (float) (kPanelX + kPanelW));

    // pie
    g.setFont (analog::serif (14.0f, juce::Font::italic));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawText ("Made By ORYMY And YMY Records", juce::Rectangle<int> (1, h - 39, w, 22), juce::Justification::centred);
    g.setColour (analog::gold);
    g.drawText ("Made By ORYMY And YMY Records", juce::Rectangle<int> (0, h - 40, w, 22), juce::Justification::centred);
}

void TarareoEditor::paint (juce::Graphics& g)
{
    if (backdrop.isNull() || backdrop.getWidth() != getWidth() || backdrop.getHeight() != getHeight())
    {
        backdrop = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics bg (backdrop);
        paintBackdrop (bg, getWidth(), getHeight());
    }
    g.drawImageAt (backdrop, 0, 0);

    // etiquetas alrededor de la perilla de genero
    const auto c = knobArea.getCentre().toFloat();
    for (int i = 0; i < genreNames().size(); ++i)
    {
        const float a = -2.4f + (float) i / 5.0f * 4.8f;
        const float lx = c.x + std::sin (a) * 62.0f, ly = c.y - std::cos (a) * 62.0f;
        const bool sel = (i == genreIndex);
        g.setFont (analog::serif (sel ? 11.0f : 10.0f));
        g.setColour (sel ? analog::amber : analog::cream.withAlpha (0.55f));
        g.drawText (genreNames()[i], juce::Rectangle<float> (lx - 40.0f, ly - 7.0f, 80.0f, 14.0f), juce::Justification::centred);

        g.setColour (sel ? analog::amber : analog::cream.withAlpha (0.35f));
        g.fillEllipse (c.x + std::sin (a) * 45.0f - 1.5f, c.y - std::cos (a) * 45.0f - 1.5f, 3.0f, 3.0f);
    }
}

void TarareoEditor::resized()
{
    singer.setBounds (28, 28, 240, 490);

    knobArea = { kPanelX, 118, 170, 140 };
    genreKnob.setBounds (knobArea.withSizeKeepingCentre (76, 76));
    vu.setBounds (kPanelX + 190, 118, kPanelW - 190, 140);

    genreLcd.setBounds (kPanelX, 266, kPanelW, 30);

    recBtn.setBounds (kPanelX, 306, 218, 46);
    genBtn.setBounds (kPanelX + 230, 306, 218, 46);

    status.setBounds (kPanelX, 358, kPanelW, 22);
    lyricsBox.setBounds (kPanelX, 386, kPanelW, 66);
    dragZone.setBounds (kPanelX, 460, kPanelW, 44);

    backdrop = {};
}
