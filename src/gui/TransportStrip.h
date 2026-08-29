#pragma once

#include "audio/ChordEngine.h"
#include "gui/AppLookAndFeel.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace chords
{

class TransportStrip : public juce::Component
{
public:
    TransportStrip(Song& song, ChordEngine& engine);

    void refresh();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onDevice;

private:
    Song& song_;
    ChordEngine& engine_;
    juce::TextButton play_ { "Play" };
    juce::TextButton stop_ { "Stop" };
    juce::ToggleButton loop_ { "Loop" };
    juce::Label bpmLabel_ { {}, "BPM" };
    juce::Slider bpm_;
    juce::TextButton device_ { "Device" };
};

} // namespace chords
