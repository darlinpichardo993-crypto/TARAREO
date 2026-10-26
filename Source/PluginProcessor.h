#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <thread>

class TarareoProcessor : public juce::AudioProcessor
{
public:
    TarareoProcessor();
    ~TarareoProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TARAREO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Flujo de trabajo
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording.load(); }
    void generate();                      // envia el tarareo al servidor (hilo aparte)

    juce::String getStatus() const  { const juce::ScopedLock l (lock); return status; }
    juce::String getLyrics() const  { const juce::ScopedLock l (lock); return lyrics; }
    juce::File   getLastFile() const{ const juce::ScopedLock l (lock); return lastFile; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void setStatus (const juce::String& s) { const juce::ScopedLock l (lock); status = s; }

    juce::AudioBuffer<float> recorded;
    std::atomic<int> writePos { 0 };
    std::atomic<bool> recording { false };
    double sampleRateHz = 44100.0;

    mutable juce::CriticalSection lock;
    juce::String status { "Listo. Pulsa GRABAR y tararea." }, lyrics;
    juce::File lastFile;
    std::thread worker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TarareoProcessor)
};
