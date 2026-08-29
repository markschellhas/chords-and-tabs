#include "audio/ChordEngine.h"

#include <algorithm>
#include <cmath>

namespace chords
{
namespace
{

struct InstrumentSound final : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

struct Timbre
{
    double h2 = 0.0;
    double h3 = 0.0;
    double h4 = 0.0;
    double h5 = 0.0;
    double tine = 0.0;      // extra inharmonic partial (electric piano)
    double tineRatio = 12.0;
    double attackSec = 0.012;
    double decaySec = 0.4;
    double sustain = 1.0;
    double releaseSec = 0.18;
    double level = 0.18;
};

Timbre timbreFor(Instrument instrument)
{
    switch (instrument)
    {
        case Instrument::Piano:
            return { 0.42, 0.20, 0.10, 0.05, 0.0, 12.0, 0.005, 0.55, 0.0, 0.22, 0.22 };
        case Instrument::ElectricPiano:
            return { 0.28, 0.06, 0.12, 0.0, 0.22, 14.0, 0.008, 0.85, 0.18, 0.28, 0.17 };
        case Instrument::Organ:
            return { 0.0, 0.45, 0.0, 0.22, 0.0, 12.0, 0.004, 0.0, 1.0, 0.06, 0.14 };
        case Instrument::Pad:
            return { 0.18, 0.07, 0.0, 0.0, 0.0, 12.0, 0.012, 0.0, 1.0, 0.18, 0.18 };
        case Instrument::Strings:
            return { 0.30, 0.14, 0.05, 0.0, 0.0, 12.0, 0.090, 0.0, 1.0, 0.35, 0.12 };
    }
    return {};
}

struct InstrumentVoice final : public juce::SynthesiserVoice
{
    explicit InstrumentVoice(std::atomic<int>& instrument)
        : instrument_(instrument) {}

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<InstrumentSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        const auto inst = instrumentFromIndex(instrument_.load(std::memory_order_acquire));
        timbre_ = timbreFor(inst);

        const auto cycles = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) / getSampleRate();
        delta_ = cycles * juce::MathConstants<double>::twoPi;
        angle_ = 0.0;
        level_ = velocity * timbre_.level;
        env_ = 0.0;
        releasing_ = false;
        decaying_ = false;

        const double sr = getSampleRate();
        attack_ = 1.0 - std::exp(-1.0 / (std::max(0.001, timbre_.attackSec) * sr));
        decay_ = 1.0 - std::exp(-1.0 / (std::max(0.02, timbre_.decaySec) * sr));
        release_ = std::exp(-1.0 / (std::max(0.02, timbre_.releaseSec) * sr));
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
            releasing_ = true;
        else
        {
            env_ = 0.0;
            delta_ = 0.0;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    using juce::SynthesiserVoice::renderNextBlock;

    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (juce::approximatelyEqual(delta_, 0.0))
            return;

        while (--numSamples >= 0)
        {
            if (releasing_)
            {
                env_ *= release_;
                if (env_ < 0.001)
                {
                    env_ = 0.0;
                    delta_ = 0.0;
                    clearCurrentNote();
                    break;
                }
            }
            else if (! decaying_)
            {
                env_ += attack_ * (1.0 - env_);
                if (env_ > 0.995)
                {
                    env_ = 1.0;
                    decaying_ = true;
                }
            }
            else if (timbre_.sustain < 0.999)
            {
                env_ += decay_ * (timbre_.sustain - env_);
            }

            const auto s = std::sin(angle_);
            const auto s2 = std::sin(angle_ * 2.0);
            const auto s3 = std::sin(angle_ * 3.0);
            const auto s4 = std::sin(angle_ * 4.0);
            const auto s5 = std::sin(angle_ * 5.0);
            const auto tine = timbre_.tine > 0.0
                ? std::sin(angle_ * timbre_.tineRatio) * timbre_.tine
                : 0.0;
            const double mix = s
                + timbre_.h2 * s2
                + timbre_.h3 * s3
                + timbre_.h4 * s4
                + timbre_.h5 * s5
                + tine;
            const float sample = static_cast<float>(mix * level_ * env_);

            for (int ch = 0; ch < output.getNumChannels(); ++ch)
                output.addSample(ch, startSample, sample);

            angle_ += delta_;
            ++startSample;
        }
    }

private:
    std::atomic<int>& instrument_;
    Timbre timbre_;
    double angle_ = 0.0;
    double delta_ = 0.0;
    double level_ = 0.0;
    double env_ = 0.0;
    double attack_ = 0.01;
    double decay_ = 0.01;
    double release_ = 0.99;
    bool releasing_ = false;
    bool decaying_ = false;
};

} // namespace

ChordEngine::ChordEngine()
{
    synth_.addSound(new InstrumentSound());
    for (int i = 0; i < 16; ++i)
        synth_.addVoice(new InstrumentVoice(instrument_));

    for (auto& n : notes_)
        n.store(-1, std::memory_order_relaxed);
}

void ChordEngine::setInstrument(Instrument instrument)
{
    instrument_.store(static_cast<int>(instrument), std::memory_order_release);
    retrigger_.store(true, std::memory_order_release);
}

void ChordEngine::cycleInstrument(int delta)
{
    setInstrument(chords::cycleInstrument(instrument(), delta));
}

Instrument ChordEngine::instrument() const
{
    return instrumentFromIndex(instrument_.load(std::memory_order_acquire));
}

const char* ChordEngine::instrumentName() const
{
    return chords::instrumentName(instrument());
}

void ChordEngine::setSong(const Song& song)
{
    const juce::ScopedLock sl(lock_);
    rebuildLocked(song);
}

void ChordEngine::rebuildLocked(const Song& song)
{
    events_ = buildTimeline(song);
    lengthBeats_ = timelineLengthBeats(events_);
    bpm_.store(song.bpm(), std::memory_order_release);
}

void ChordEngine::setBpm(double bpm)
{
    bpm_.store(std::clamp(bpm, 40.0, 240.0), std::memory_order_release);
}

void ChordEngine::setLooping(bool shouldLoop)
{
    looping_.store(shouldLoop, std::memory_order_release);
}

void ChordEngine::play()
{
    playing_.store(true, std::memory_order_release);
}

void ChordEngine::stop()
{
    playing_.store(false, std::memory_order_release);
    currentBeat_.store(0.0, std::memory_order_release);
}

void ChordEngine::togglePlay()
{
    if (isPlaying())
        stop();
    else
        play();
}

std::optional<PlayEvent> ChordEngine::currentEvent() const
{
    const juce::ScopedLock sl(lock_);
    const auto* e = eventAt(events_, currentBeat_.load(std::memory_order_acquire));
    if (e == nullptr)
        return std::nullopt;
    return *e;
}

std::array<int, 3> ChordEngine::soundingNotes() const
{
    return { notes_[0].load(std::memory_order_acquire),
             notes_[1].load(std::memory_order_acquire),
             notes_[2].load(std::memory_order_acquire) };
}

void ChordEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sr = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    sampleRate_.store(sr, std::memory_order_release);
    synth_.setCurrentPlaybackSampleRate(sr);
}

void ChordEngine::audioDeviceStopped()
{
    synth_.allNotesOff(1, false);
}

void ChordEngine::allNotesOff(juce::MidiBuffer& midi, int sampleOffset)
{
    for (auto& n : notes_)
    {
        const int note = n.exchange(-1, std::memory_order_acq_rel);
        if (note >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, note), sampleOffset);
    }
    numSounding_.store(0, std::memory_order_release);
}

void ChordEngine::startChord(const Chord& chord, juce::MidiBuffer& midi, int sampleOffset)
{
    const auto notes = chord.midiNotes(4);
    int count = 0;
    for (int i = 0; i < 3; ++i)
    {
        midi.addEvent(juce::MidiMessage::noteOn(1, notes[static_cast<size_t>(i)], 0.72f), sampleOffset);
        notes_[static_cast<size_t>(i)].store(notes[static_cast<size_t>(i)], std::memory_order_release);
        ++count;
    }
    numSounding_.store(count, std::memory_order_release);
}

void ChordEngine::applyEvent(const PlayEvent* event, juce::MidiBuffer& midi, int sampleOffset)
{
    if (event == nullptr)
    {
        if (! lastRest_ || lastSlot_ != -1)
        {
            allNotesOff(midi, sampleOffset);
            lastRest_ = true;
            lastSection_ = lastMeasure_ = lastSlot_ = -1;
        }
        return;
    }

    const bool same = event->sectionIndex == lastSection_
                   && event->measureIndex == lastMeasure_
                   && event->slotIndex == lastSlot_;
    if (same)
        return;

    allNotesOff(midi, sampleOffset);
    lastSection_ = event->sectionIndex;
    lastMeasure_ = event->measureIndex;
    lastSlot_ = event->slotIndex;
    lastRest_ = event->rest;

    if (! event->rest)
        startChord(event->chord, midi, sampleOffset);
}

void ChordEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);

    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    juce::MidiBuffer midi;

    const double sr = sampleRate_.load(std::memory_order_acquire);
    const double bpm = bpm_.load(std::memory_order_acquire);
    const bool playing = playing_.load(std::memory_order_acquire);
    const double beatsPerSample = (sr > 0.0) ? (bpm / 60.0) / sr : 0.0;

    double beat = currentBeat_.load(std::memory_order_acquire);

    {
        const juce::ScopedLock sl(lock_);

        if (retrigger_.exchange(false, std::memory_order_acq_rel))
        {
            lastSection_ = lastMeasure_ = lastSlot_ = -1;
            lastRest_ = true;
        }

        if (! playing)
        {
            applyEvent(nullptr, midi, 0);
            beat = currentBeat_.load(std::memory_order_acquire);
        }
        else if (lengthBeats_ <= 0.0)
        {
            applyEvent(nullptr, midi, 0);
        }
        else
        {
            if (beat >= lengthBeats_)
            {
                if (looping_.load(std::memory_order_acquire))
                    beat = std::fmod(beat, lengthBeats_);
                else
                {
                    playing_.store(false, std::memory_order_release);
                    beat = 0.0;
                    applyEvent(nullptr, midi, 0);
                }
            }

            if (playing_.load(std::memory_order_acquire))
            {
                const auto* ev = eventAt(events_, beat);
                applyEvent(ev, midi, 0);
                beat += static_cast<double>(numSamples) * beatsPerSample;

                if (beat >= lengthBeats_)
                {
                    if (looping_.load(std::memory_order_acquire) && lengthBeats_ > 0.0)
                        beat = std::fmod(beat, lengthBeats_);
                    else
                    {
                        playing_.store(false, std::memory_order_release);
                        beat = 0.0;
                    }
                }
            }
        }
    }

    currentBeat_.store(beat, std::memory_order_release);
    synth_.renderNextBlock(buffer, midi, 0, numSamples);
}

} // namespace chords
