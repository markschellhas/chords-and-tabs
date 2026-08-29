#include "gui/MeasureComponent.h"

namespace chords
{

MeasureComponent::MeasureComponent(Song& song, int section, int measure)
    : song_(song), section_(section), measure_(measure)
{
    rebuild();
}

void MeasureComponent::setPlayingSlot(int slotIndex)
{
    playingSlot_ = slotIndex;
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
        slots_[static_cast<size_t>(i)]->setPlaying(i == playingSlot_);
}

void MeasureComponent::rebuild()
{
    slots_.clear();

    if (section_ < 0 || section_ >= static_cast<int>(song_.sections().size()))
        return;
    const auto& section = song_.sections()[static_cast<size_t>(section_)];
    if (measure_ < 0 || measure_ >= static_cast<int>(section.measures.size()))
        return;

    const auto& measure = section.measures[static_cast<size_t>(measure_)];
    for (int i = 0; i < static_cast<int>(measure.slots.size()); ++i)
    {
        auto slot = std::make_unique<ChordSlotComponent>(song_, section_, measure_, i);
        slot->onSelected = [this, i] {
            if (onSlotSelected)
                onSlotSelected(i);
        };
        addAndMakeVisible(*slot);
        slots_.push_back(std::move(slot));
    }

    setPlayingSlot(playingSlot_);
    resized();
}

void MeasureComponent::resized()
{
    auto r = getLocalBounds();
    const int n = juce::jmax(1, static_cast<int>(slots_.size()));
    const int gap = 3;
    const int w = juce::jmax(28, (r.getWidth() - gap * (n - 1)) / n);
    for (auto& slot : slots_)
    {
        slot->setBounds(r.removeFromLeft(w));
        r.removeFromLeft(gap);
    }
}

} // namespace chords
