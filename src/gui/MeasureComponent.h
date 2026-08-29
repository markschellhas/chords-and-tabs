#pragma once

#include "gui/ChordSlotComponent.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace chords
{

class MeasureComponent : public juce::Component
{
public:
    MeasureComponent(Song& song, int section, int measure);

    void setPlayingSlot(int slotIndex);
    void rebuild();
    static constexpr int kHeight = 52;

    void paint(juce::Graphics&) override {}
    void resized() override;

    std::function<void(int slot)> onSlotSelected;

private:
    Song& song_;
    int section_ = 0;
    int measure_ = 0;
    int playingSlot_ = -1;
    std::vector<std::unique_ptr<ChordSlotComponent>> slots_;
};

} // namespace chords
