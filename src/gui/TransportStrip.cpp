#include "gui/TransportStrip.h"

#include <cmath>

namespace chords
{

TransportStrip::TransportStrip(Song& song, ChordEngine& engine)
    : song_(song), engine_(engine)
{
    play_.onClick = [this] { engine_.play(); };
    stop_.onClick = [this] { engine_.stop(); };
    loop_.setToggleState(engine_.isLooping(), juce::dontSendNotification);
    loop_.onClick = [this] { engine_.setLooping(loop_.getToggleState()); };

    bpm_.setRange(40.0, 240.0, 1.0);
    bpm_.setValue(song_.bpm(), juce::dontSendNotification);
    bpm_.setSliderStyle(juce::Slider::LinearHorizontal);
    bpm_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
    bpm_.onValueChange = [this] {
        song_.setBpm(bpm_.getValue());
        engine_.setBpm(song_.bpm());
    };

    bpmLabel_.setJustificationType(juce::Justification::centredRight);
    device_.onClick = [this] {
        if (onDevice)
            onDevice();
    };

    addAndMakeVisible(play_);
    addAndMakeVisible(stop_);
    addAndMakeVisible(loop_);
    addAndMakeVisible(bpmLabel_);
    addAndMakeVisible(bpm_);
    addAndMakeVisible(device_);
}

void TransportStrip::refresh()
{
    const bool playing = engine_.isPlaying();
    play_.setToggleState(playing, juce::dontSendNotification);
    play_.setButtonText(playing ? "Playing" : "Play");
    if (std::abs(bpm_.getValue() - song_.bpm()) > 0.5)
        bpm_.setValue(song_.bpm(), juce::dontSendNotification);
}

void TransportStrip::paint(juce::Graphics&) {}

void TransportStrip::resized()
{
    auto r = getLocalBounds().reduced(6, 6);
    play_.setBounds(r.removeFromLeft(72));
    r.removeFromLeft(6);
    stop_.setBounds(r.removeFromLeft(72));
    r.removeFromLeft(8);
    loop_.setBounds(r.removeFromLeft(64));
    device_.setBounds(r.removeFromRight(80));
    r.removeFromRight(8);
    bpm_.setBounds(r.removeFromRight(160));
    bpmLabel_.setBounds(r.removeFromRight(40));
}

} // namespace chords
