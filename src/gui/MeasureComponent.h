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

    void setPlayingSlot(int slotIndex, float progress = 0.0f);
    void rebuild();
    static constexpr int kHeight = 52;

    void paint(juce::Graphics&) override {}
    void resized() override;

    std::function<void(int slot)> onSlotSelected;
    std::function<void(int slot)> onSlotAudition;

private:
    void syncSlots();
    void wireSlot(ChordSlotComponent& slot);
    void handleResize(int slotIndex, bool fromLeft, int x);
    int spanFromResizeX(int slotIndex, bool fromLeft, int x) const;

    Song& song_;
    int section_ = 0;
    int measure_ = 0;
    int playingSlot_ = -1;
    float playProgress_ = 0.0f;
    std::vector<std::unique_ptr<ChordSlotComponent>> slots_;
};

} // namespace chords
