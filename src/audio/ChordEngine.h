#pragma once

#include "audio/Instruments.h"
#include "model/Song.h"
#include "model/Timeline.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <optional>

namespace chords
{

class ChordEngine : public juce::AudioIODeviceCallback
{
public:
    ChordEngine();
    ~ChordEngine() override = default;

    void setSong(const Song& song);
    void setBpm(double bpm);
    void setLooping(bool shouldLoop);
    bool isLooping() const { return looping_.load(std::memory_order_acquire); }

    void play();
    void stop();
    void togglePlay();
    bool isPlaying() const { return playing_.load(std::memory_order_acquire); }

    /** Audition one chord for `durationBeats` at the current BPM. Stops song playback. */
    void playChord(const Chord& chord, double durationBeats);
    void cancelPreview();
    bool isPreviewing() const { return previewing_.load(std::memory_order_acquire); }
    double previewProgress() const;

    double currentBeat() const { return currentBeat_.load(std::memory_order_acquire); }
    std::optional<PlayEvent> currentEvent() const;
    double currentEventProgress() const;
    std::array<int, 3> soundingNotes() const;
    bool hasSoundingNotes() const { return numSounding_.load(std::memory_order_acquire) > 0; }

    double sampleRate() const { return sampleRate_.load(std::memory_order_acquire); }

    void setInstrument(Instrument instrument);
    void cycleInstrument(int delta);
    Instrument instrument() const;
    const char* instrumentName() const;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    void rebuildLocked(const Song& song);
    void allNotesOff(juce::MidiBuffer& midi, int sampleOffset);
    void startChord(const Chord& chord, juce::MidiBuffer& midi, int sampleOffset);
    void applyEvent(const PlayEvent* event, juce::MidiBuffer& midi, int sampleOffset);
    void applyPreview(juce::MidiBuffer& midi, int numSamples, double beatsPerSample);

    std::atomic<int> instrument_ { static_cast<int>(Instrument::Piano) };
    juce::Synthesiser synth_;
    juce::CriticalSection lock_;
    std::vector<PlayEvent> events_;
    double lengthBeats_ = 0.0;

    std::atomic<double> bpm_ { 120.0 };
    std::atomic<double> sampleRate_ { 44100.0 };
    std::atomic<double> currentBeat_ { 0.0 };
    std::atomic<bool> playing_ { false };
    std::atomic<bool> looping_ { true };
    std::atomic<int> numSounding_ { 0 };
    std::atomic<int> sounding_ { 0 }; // packed? we'll keep an array behind the lock for event
    std::array<std::atomic<int>, 3> notes_;
    std::atomic<bool> retrigger_ { false };

    std::atomic<bool> previewing_ { false };
    std::atomic<double> previewBeat_ { 0.0 };
    std::atomic<double> previewDuration_ { 0.0 };
    Chord previewChord_ {};
    bool previewRetrigger_ = false;

    int lastSection_ = -1;
    int lastMeasure_ = -1;
    int lastSlot_ = -1;
    bool lastRest_ = true;
};

} // namespace chords
