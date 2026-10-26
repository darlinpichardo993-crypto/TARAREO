#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "genre", 1 }, "Genero",
        juce::StringArray { "Bachata", "Merengue", "Reggaeton", "Trap", "R&B", "Pop" }, 2));
    return l;
}

TarareoProcessor::TarareoProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
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
    recorded.setSize (1, (int) (sr * 30.0));   // hasta 30 s de tarareo
    recorded.clear();
    writePos = 0;
}

bool TarareoProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void TarareoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (! recording.load() || recorded.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;                                 // el audio pasa sin tocar

    const int pos  = writePos.load();
    const int room = recorded.getNumSamples() - pos;
    if (room <= 0) { recording = false; return; }

    const int n = juce::jmin (buffer.getNumSamples(), room);
    recorded.copyFrom (0, pos, buffer, 0, 0, n);
    writePos = pos + n;
}

void TarareoProcessor::startRecording()
{
    writePos = 0;
    recorded.clear();
    recording = true;
    setStatus ("Grabando...");
}

void TarareoProcessor::stopRecording()
{
    if (recording.exchange (false))
        setStatus ("Grabacion lista. Pulsa GENERAR.");
}

void TarareoProcessor::generate()
{
    stopRecording();
    const int n = writePos.load();
    if (n < (int) (sampleRateHz * 0.5)) { setStatus ("Graba un tarareo primero (min. 0.5 s)."); return; }

    juce::AudioBuffer<float> copy (1, n);
    copy.copyFrom (0, 0, recorded, 0, 0, n);
    const double sr = sampleRateHz;
    const juce::String genre = apvts.getParameter ("genre")->getCurrentValueAsText();

    if (worker.joinable()) worker.join();
    setStatus ("Analizando tarareo y escribiendo letra...");

    worker = std::thread ([this, copy, sr, genre]
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("tarareo_in.wav");
        tmp.deleteFile();
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::FileOutputStream> fos (tmp.createOutputStream());
            if (fos == nullptr) { setStatus ("No pude crear el archivo temporal."); return; }
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (fos.get(), sr, 1, 16, {}, 0));
            if (w == nullptr) { setStatus ("Error al escribir el WAV."); return; }
            fos.release();
            w->writeFromAudioSampleBuffer (copy, 0, copy.getNumSamples());
        }

        auto server = juce::SystemStats::getEnvironmentVariable ("TARAREO_SERVER", "http://127.0.0.1:8000");
        juce::URL url (server + "/generate");
        url = url.withFileToUpload ("file", tmp, "audio/wav").withParameter ("genre", genre);

        juce::StringPairArray headers;
        auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                        .withConnectionTimeoutMs (180000)
                        .withResponseHeaders (&headers);

        if (auto in = url.createInputStream (opts))
        {
            juce::MemoryBlock mb;
            in->readIntoMemoryBlock (mb);
            if (mb.getSize() < 100) { setStatus ("El servidor no devolvio audio."); return; }

            auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("TARAREO");
            dir.createDirectory();
            auto out = dir.getChildFile ("tarareo_" + juce::String (juce::Time::currentTimeMillis()) + ".wav");
            out.replaceWithData (mb.getData(), mb.getSize());

            const juce::ScopedLock l (lock);
            lastFile = out;
            lyrics = juce::URL::removeEscapeChars (headers.getValue ("X-Lyrics", ""));
            status = "Listo. Arrastra el audio a FL Studio. Tonalidad: "
                   + juce::URL::removeEscapeChars (headers.getValue ("X-Key", "?"));
        }
        else
        {
            setStatus ("No pude conectar con el servidor (" + server + ").");
        }
    });
}

juce::AudioProcessorEditor* TarareoProcessor::createEditor() { return new TarareoEditor (*this); }

void TarareoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void TarareoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TarareoProcessor(); }
