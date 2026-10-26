#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float baseW = 1100.0f, baseH = 720.0f;
    void setupCombo (juce::ComboBox& combo, const juce::StringArray& options,
                     juce::LookAndFeel& laf)
    {
        combo.setLookAndFeel (&laf);
        combo.addItemList (options, 1);
        combo.setJustificationType (juce::Justification::centredLeft);
    }
}

TarareoEditor::TarareoEditor (TarareoProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setupCombo (genreBox, { "Bachata", "Merengue", "Reggaetón", "Trap", "R&B", "Pop",
                            "Salsa", "Dembow", "Bolero", "Soul" }, laf);
    setupCombo (sectionBox, { "Verso", "Pre-coro", "Coro", "Puente", "Outro" }, laf);
    setupCombo (voiceBox, { "Masculina", "Femenina" }, laf);
    setupCombo (rangeBox, { "Bajo", "Barítono", "Tenor", "Contralto", "Alto", "Mezzosoprano", "Soprano" }, laf);
    setupCombo (bpmModeBox, { "Sincronizar con FL Studio", "BPM manual" }, laf);
    setupCombo (zoomBox, { "100%", "125%", "150%" }, laf);

    genreAttach = std::make_unique<ComboAttachment> (proc.apvts, "genre", genreBox);
    sectionAttach = std::make_unique<ComboAttachment> (proc.apvts, "section", sectionBox);
    voiceAttach = std::make_unique<ComboAttachment> (proc.apvts, "voice", voiceBox);
    rangeAttach = std::make_unique<ComboAttachment> (proc.apvts, "range", rangeBox);
    bpmModeAttach = std::make_unique<ComboAttachment> (proc.apvts, "bpmMode", bpmModeBox);

    for (auto* box : { &genreBox, &sectionBox, &voiceBox, &rangeBox, &bpmModeBox, &zoomBox })
        addAndMakeVisible (*box);
    zoomBox.setSelectedId (1, juce::dontSendNotification);
    zoomBox.onChange = [this]
    {
        const float zoom = zoomBox.getSelectedId() == 2 ? 1.25f
                         : zoomBox.getSelectedId() == 3 ? 1.5f : 1.0f;
        setSize ((int) (baseW * zoom), (int) (baseH * zoom));
    };

    for (auto* s : { &bpmSlider, &creativitySlider })
    {
        s->setLookAndFeel (&laf);
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 26);
        s->setColour (juce::Slider::textBoxTextColourId, neon::white);
        s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff111725));
        s->setColour (juce::Slider::textBoxOutlineColourId, neon::border);
        addAndMakeVisible (*s);
    }
    bpmAttach = std::make_unique<SliderAttachment> (proc.apvts, "bpm", bpmSlider);
    creativityAttach = std::make_unique<SliderAttachment> (proc.apvts, "creativity", creativitySlider);
    bpmSlider.setTextValueSuffix (" BPM");
    creativitySlider.setTextValueSuffix (" %");

    themeBox.setMultiLine (false);
    themeBox.setTextToShowWhenEmpty ("Ej.: Me perdi pensando en ti...", neon::muted);
    themeBox.setText (proc.getTheme(), false);
    themeBox.onTextChange = [this] { proc.setTheme (themeBox.getText()); };
    themeBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0c1120));
    themeBox.setColour (juce::TextEditor::outlineColourId, neon::border);
    themeBox.setColour (juce::TextEditor::focusedOutlineColourId, neon::cyan);
    themeBox.setColour (juce::TextEditor::textColourId, neon::white);
    themeBox.setColour (juce::TextEditor::highlightColourId, neon::violet);
    addAndMakeVisible (themeBox);

    lyricsBox.setMultiLine (true);
    lyricsBox.setReturnKeyStartsNewLine (true);
    lyricsBox.setReadOnly (true);
    lyricsBox.setScrollbarsShown (true);
    lyricsBox.setTextToShowWhenEmpty ("Tu composicion aparecera aqui, con frases completas...", neon::muted);
    lyricsBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b1020));
    lyricsBox.setColour (juce::TextEditor::outlineColourId, neon::border);
    lyricsBox.setColour (juce::TextEditor::focusedOutlineColourId, neon::violet);
    lyricsBox.setColour (juce::TextEditor::textColourId, neon::white);
    lyricsBox.setColour (juce::TextEditor::highlightColourId, neon::violet.withAlpha (0.65f));
    addAndMakeVisible (lyricsBox);

    recBtn.setLookAndFeel (&laf);
    genBtn.setLookAndFeel (&laf);
    genBtn.setComponentID ("generate");
    recBtn.setClickingTogglesState (true);
    recBtn.onClick = [this]
    {
        if (recBtn.getToggleState()) proc.startRecording();
        else proc.stopRecording();
    };
    genBtn.onClick = [this]
    {
        recBtn.setToggleState (false, juce::dontSendNotification);
        proc.generate();
    };
    addAndMakeVisible (recBtn);
    addAndMakeVisible (genBtn);

    status.setJustificationType (juce::Justification::centredLeft);
    status.setColour (juce::Label::textColourId, neon::muted);
    status.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (status);
    addAndMakeVisible (singer);
    addAndMakeVisible (dragZone);
    singer.setFemale (proc.apvts.getRawParameterValue ("voice")->load() >= 0.5f);

    setResizable (true, true);
    setResizeLimits (880, 576, 1650, 1080);
    setSize ((int) baseW, (int) baseH);
    startTimerHz (24);
}

TarareoEditor::~TarareoEditor()
{
    stopTimer();
    for (auto* box : { &genreBox, &sectionBox, &voiceBox, &rangeBox, &bpmModeBox, &zoomBox })
        box->setLookAndFeel (nullptr);
    for (auto* slider : { &bpmSlider, &creativitySlider })
        slider->setLookAndFeel (nullptr);
    recBtn.setLookAndFeel (nullptr);
    genBtn.setLookAndFeel (nullptr);
}

juce::Rectangle<int> TarareoEditor::scaleBounds (float x, float y, float w, float h) const
{
    return { (int) std::round (x * getWidth() / baseW),
             (int) std::round (y * getHeight() / baseH),
             (int) std::round (w * getWidth() / baseW),
             (int) std::round (h * getHeight() / baseH) };
}

void TarareoEditor::paintLabel (juce::Graphics& g, const juce::String& text,
                                float x, float y, float w, juce::Colour colour)
{
    const float scale = juce::jmin (getWidth() / baseW, getHeight() / baseH);
    g.setFont (neon::font (12.0f * scale, juce::Font::bold));
    g.setColour (colour);
    g.drawText (text, scaleBounds (x, y, w, 22), juce::Justification::centredLeft);
}

void TarareoEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff11172b), 0, 0,
                                             neon::bg, (float) getWidth(), (float) getHeight(), false));
    g.fillAll();
    g.setColour (neon::cyan.withAlpha (0.13f));
    for (int x = 0; x < getWidth(); x += 40) g.drawVerticalLine (x, 0.0f, (float) getHeight());

    const float s = juce::jmin (getWidth() / baseW, getHeight() / baseH);
    g.setColour (neon::white);
    g.setFont (neon::font (42.0f * s, juce::Font::bold));
    g.drawText ("TARAREO", scaleBounds (30, 20, 302, 62), juce::Justification::centredLeft);
    g.setColour (neon::pink);
    g.fillRoundedRectangle (scaleBounds (295, 70, 92, 4).toFloat(), 2.0f);
    paintLabel (g, "AI SONGWRITER / V3 PROFESSIONAL", 366, 30, 390, neon::cyan);
    paintLabel (g, "ESCALA", 905, 29, 105);

    neon::panelBackground (g, scaleBounds (294, 105, 458, 560).toFloat(), neon::violet);
    neon::panelBackground (g, scaleBounds (773, 105, 302, 560).toFloat(), neon::pink);
    paintLabel (g, "IDEA / TEMA DE LA CANCION", 312, 127, 370);
    paintLabel (g, "LETRA GENERADA CON IA", 312, 345, 380, neon::cyan);

    paintLabel (g, "AJUSTES CREATIVOS", 792, 125, 245, neon::pink);
    paintLabel (g, "GENERO MUSICAL", 792, 166, 240);
    paintLabel (g, "PARTE DE LA CANCION", 792, 230, 240);
    paintLabel (g, "MODELO DE VOZ", 792, 294, 240);
    paintLabel (g, "RANGO VOCAL", 792, 358, 240);
    paintLabel (g, "SINCRONIZACION DEL TEMPO", 792, 422, 260);
    paintLabel (g, "BPM", 792, 494, 180);
    paintLabel (g, "CREATIVIDAD", 792, 558, 200);

    g.setColour (neon::violet.withAlpha (0.34f));
    g.drawHorizontalLine (scaleBounds (0, 677, 0, 0).getY(),
                          (float) scaleBounds (29, 0, 0, 0).getX(),
                          (float) scaleBounds (1072, 0, 0, 0).getX());
    g.setFont (neon::font (13.0f * s, juce::Font::bold));
    g.setColour (neon::white.withAlpha (0.83f));
    g.drawText ("Made By ORYMY And YMY Records", scaleBounds (25, 681, 600, 29),
                juce::Justification::centredLeft);
    g.setColour (neon::muted);
    g.drawText ("VST3  /  WINDOWS  /  FL STUDIO", scaleBounds (728, 681, 349, 29),
                juce::Justification::centredRight);
}

void TarareoEditor::resized()
{
    singer.setBounds (scaleBounds (25, 105, 246, 560));
    zoomBox.setBounds (scaleBounds (969, 41, 104, 32));
    themeBox.setBounds (scaleBounds (312, 158, 420, 47));
    recBtn.setBounds (scaleBounds (312, 224, 198, 48));
    genBtn.setBounds (scaleBounds (527, 224, 205, 48));
    status.setBounds (scaleBounds (312, 285, 422, 44));
    lyricsBox.setBounds (scaleBounds (312, 377, 420, 205));
    dragZone.setBounds (scaleBounds (312, 603, 420, 44));

    genreBox.setBounds (scaleBounds (791, 188, 264, 36));
    sectionBox.setBounds (scaleBounds (791, 252, 264, 36));
    voiceBox.setBounds (scaleBounds (791, 316, 264, 36));
    rangeBox.setBounds (scaleBounds (791, 380, 264, 36));
    bpmModeBox.setBounds (scaleBounds (791, 444, 264, 36));
    bpmSlider.setBounds (scaleBounds (791, 519, 264, 35));
    creativitySlider.setBounds (scaleBounds (791, 584, 264, 35));

    const float scale = juce::jmin (getWidth() / baseW, getHeight() / baseH);
    lyricsBox.setFont (neon::font (21.0f * scale));
    themeBox.setFont (neon::font (15.0f * scale));
    status.setFont (neon::font (13.0f * scale));
}

void TarareoEditor::timerCallback()
{
    singer.setLevel (proc.getInputLevel());
    const int voice = (int) std::round (proc.apvts.getRawParameterValue ("voice")->load());
    if (voice != voiceIndex)
    {
        voiceIndex = voice;
        singer.setFemale (voice == 1);
    }

    const auto message = proc.getStatus();
    if (message != status.getText()) status.setText (message, juce::dontSendNotification);
    const auto lyric = proc.getLyrics();
    if (lyric != lyricsBox.getText()) lyricsBox.setText (lyric, false);

    const bool recording = proc.isRecording();
    if (recBtn.getToggleState() != recording)
        recBtn.setToggleState (recording, juce::dontSendNotification);
    recBtn.setButtonText (recording ? "■ DETENER" : "● GRABAR");
    genBtn.setEnabled (! proc.isGenerating());
    bpmSlider.setEnabled (proc.apvts.getRawParameterValue ("bpmMode")->load() >= 0.5f);

    const auto path = proc.getLastFile().getFullPathName();
    const auto mode = proc.getAudioMode();
    if (path != lastPath || mode != lastAudioMode)
    {
        lastPath = path;
        lastAudioMode = mode;
        dragZone.repaint();
    }
}
