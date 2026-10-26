#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstring>

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "genre", 1 }, "Genero",
        juce::StringArray { "Bachata", "Merengue", "Reggaetón", "Trap", "R&B", "Pop",
                            "Salsa", "Dembow", "Bolero", "Soul" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "section", 1 }, "Seccion",
        juce::StringArray { "Verso", "Pre-coro", "Coro", "Puente", "Outro" }, 2));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "voice", 1 }, "Voz",
        juce::StringArray { "Masculina", "Femenina" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "range", 1 }, "Registro vocal",
        juce::StringArray { "Bajo", "Barítono", "Tenor", "Contralto", "Alto",
                            "Mezzosoprano", "Soprano" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "bpmMode", 1 }, "BPM fuente",
        juce::StringArray { "FL Studio", "Manual" }, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "bpm", 1 }, "BPM manual", 60, 220, 132));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "creativity", 1 }, "Creatividad", 0, 100, 65));
    return layout;
}

TarareoProcessor::TarareoProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", makeLayout())
{
}

TarareoProcessor::~TarareoProcessor()
{
    if (worker.joinable()) worker.join();
}

void TarareoProcessor::prepareToPlay (double sr, int)
{
    sampleRateHz = sr;
    recorded.setSize (1, (int) (sr * 30.0));
    recorded.clear();
    writePos = 0;
}

bool TarareoProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet() == layout.getMainInputChannelSet()
        && (layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void TarareoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto tempo = position->getBpm())
                if (*tempo >= 20.0 && *tempo <= 400.0)
                    hostBpm = (float) *tempo;

    if (buffer.getNumChannels() > 0)
        inputLevel = juce::jmax (buffer.getMagnitude (0, buffer.getNumSamples()),
                                inputLevel.load() * 0.97f);

    // The plugin is a transparent pass-through. Never block the audio callback.
    if (! recording.load() || recorded.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;
    const int pos = writePos.load();
    const int room = recorded.getNumSamples() - pos;
    if (room <= 0) { recording = false; return; }
    const int samples = juce::jmin (buffer.getNumSamples(), room);
    recorded.copyFrom (0, pos, buffer, 0, 0, samples);
    writePos = pos + samples;
}

void TarareoProcessor::startRecording()
{
    if (generating.load()) { setStatus ("Espera a que termine la generación."); return; }
    writePos = 0;
    recorded.clear();
    recording = true;
    setStatus ("Grabando... tararea tu melodia.");
}

void TarareoProcessor::stopRecording()
{
    if (recording.exchange (false))
        setStatus ("Grabación lista. Pulsa COMPONER.");
}

void TarareoProcessor::generate()
{
    if (generating.exchange (true))
    {
        setStatus ("Ya hay una generación en curso.");
        return;
    }
    stopRecording();
    const int samples = writePos.load();
    if (samples < (int) (sampleRateHz * 0.5))
    {
        generating = false;
        setStatus ("Graba primero al menos medio segundo.");
        return;
    }

    juce::AudioBuffer<float> copy (1, samples);
    copy.copyFrom (0, 0, recorded, 0, 0, samples);
    const double sr = sampleRateHz;
    const auto genre = apvts.getParameter ("genre")->getCurrentValueAsText();
    const auto section = apvts.getParameter ("section")->getCurrentValueAsText();
    const auto voice = apvts.getParameter ("voice")->getCurrentValueAsText();
    const auto vocalRange = apvts.getParameter ("range")->getCurrentValueAsText();
    const int manualBpm = (int) apvts.getRawParameterValue ("bpm")->load();
    const bool useHost = apvts.getRawParameterValue ("bpmMode")->load() < 0.5f;
    const float host = hostBpm.load();
    const int bpm = useHost && host >= 60.0f && host <= 220.0f
                  ? (int) std::round (host) : manualBpm;
    const int creativity = (int) apvts.getRawParameterValue ("creativity")->load();
    const auto songTheme = getTheme();

    if (worker.joinable()) worker.join(); // previous worker already completed
    setStatus ("Analizando la melodía y escribiendo una letra original...");
    worker = std::thread ([this, copy, sr, genre, section, voice,
                           vocalRange, bpm, creativity, songTheme]
    {
        struct Completion
        {
            std::atomic<bool>& flag;
            ~Completion() { flag = false; }
        } completion { generating };

        auto tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("tarareo_" + juce::Uuid().toString() + ".wav");
        struct Cleanup
        {
            juce::File file;
            ~Cleanup() { file.deleteFile(); }
        } cleanup { tempFile };

        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::FileOutputStream> output (tempFile.createOutputStream());
            if (! output) { setStatus ("No se pudo crear el WAV temporal."); return; }
            std::unique_ptr<juce::AudioFormatWriter> writer (
                format.createWriterFor (output.get(), sr, 1, 16, {}, 0));
            if (! writer) { setStatus ("No se pudo escribir el WAV temporal."); return; }
            output.release(); // AudioFormatWriter now owns the stream
            if (! writer->writeFromAudioSampleBuffer (copy, 0, copy.getNumSamples()))
            {
                setStatus ("Fallo de escritura del WAV."); return;
            }
        }

        const auto server = juce::SystemStats::getEnvironmentVariable (
            "TARAREO_SERVER", "http://127.0.0.1:8000");
        auto url = juce::URL (server + "/generate")
                       .withFileToUpload ("file", tempFile, "audio/wav")
                       .withParameter ("genre", genre)
                       .withParameter ("section", section)
                       .withParameter ("voice", voice)
                       .withParameter ("vocal_range", vocalRange)
                       .withParameter ("bpm", juce::String (bpm))
                       .withParameter ("creativity", juce::String (creativity))
                       .withParameter ("theme", songTheme);

        juce::StringPairArray headers;
        int responseCode = 0;
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withConnectionTimeoutMs (180000)
                           .withResponseHeaders (&headers)
                           .withStatusCode (&responseCode);
        auto input = url.createInputStream (options);
        if (! input)
        {
            setStatus ("No pude conectar con el servidor: " + server);
            return;
        }

        juce::MemoryBlock bytes;
        input->readIntoMemoryBlock (bytes);
        if (responseCode < 200 || responseCode >= 300)
        {
            auto body = juce::String::fromUTF8 (
                (const char*) bytes.getData(), (int) juce::jmin ((size_t) 500, bytes.getSize()));
            auto json = juce::JSON::parse (body);
            auto detail = json.getProperty ("detail", body).toString();
            setStatus ("Error de IA: " + detail.substring (0, 170));
            return;
        }
        if (bytes.getSize() < 44 || std::memcmp (bytes.getData(), "RIFF", 4) != 0)
        {
            setStatus ("El servidor no devolvió un archivo WAV válido.");
            return;
        }

        auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                       .getChildFile ("TARAREO");
        if (! dir.createDirectory())
        {
            setStatus ("No pude crear Documents/TARAREO."); return;
        }
        const auto destination = dir.getChildFile (
            "tarareo_" + juce::String (juce::Time::currentTimeMillis()) + ".wav");
        if (! destination.replaceWithData (bytes.getData(), bytes.getSize()))
        {
            setStatus ("No se pudo guardar el WAV."); return;
        }

        const auto mode = headers.getValue ("X-Audio-Mode", "guide");
        const auto key = juce::URL::removeEscapeChars (headers.getValue ("X-Key", "?"));
        const juce::ScopedLock locked (lock);
        lyrics = juce::URL::removeEscapeChars (headers.getValue ("X-Lyrics", ""));
        audioMode = mode;
        lastFile = destination;
        status = (mode == "singing" ? "VOZ IA CANTADA" : "PISTA GUÍA: FALTA MODELO DE CANTO")
                 + juce::String ("  |  Tonalidad: ") + key;
    });
}

juce::AudioProcessorEditor* TarareoProcessor::createEditor()
{
    return new TarareoEditor (*this);
}

void TarareoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("songTheme", getTheme(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TarareoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            setTheme (state.getProperty ("songTheme", "").toString());
            apvts.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TarareoProcessor();
}
