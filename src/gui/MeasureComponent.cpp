#include "gui/MeasureComponent.h"

#include <cmath>

namespace chords
{

MeasureComponent::MeasureComponent(Song& song, int section, int measure)
    : song_(song), section_(section), measure_(measure)
{
    rebuild();
}

void MeasureComponent::setPlayingSlot(int slotIndex, float progress)
{
    playingSlot_ = slotIndex;
    playProgress_ = slotIndex >= 0 ? progress : 0.0f;
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
        slots_[static_cast<size_t>(i)]->setPlaying(i == playingSlot_,
                                                  i == playingSlot_ ? playProgress_ : 0.0f);
}

void MeasureComponent::wireSlot(ChordSlotComponent& slot)
{
    slot.onResizeDrag = [this](int index, bool fromLeft, int x) {
        handleResize(index, fromLeft, x);
    };
}

void MeasureComponent::rebuild()
{
    slots_.clear();
    syncSlots();
}

void MeasureComponent::syncSlots()
{
    if (section_ < 0 || section_ >= static_cast<int>(song_.sections().size()))
    {
        slots_.clear();
        return;
    }
    const auto& section = song_.sections()[static_cast<size_t>(section_)];
    if (measure_ < 0 || measure_ >= static_cast<int>(section.measures.size()))
    {
        slots_.clear();
        return;
    }

    const auto& measure = section.measures[static_cast<size_t>(measure_)];
    const int n = static_cast<int>(measure.slots.size());

    while (static_cast<int>(slots_.size()) > n)
        slots_.pop_back();

    while (static_cast<int>(slots_.size()) < n)
    {
        const int i = static_cast<int>(slots_.size());
        auto slot = std::make_unique<ChordSlotComponent>(song_, section_, measure_, i);
        wireSlot(*slot);
        addAndMakeVisible(*slot);
        slots_.push_back(std::move(slot));
    }

    for (int i = 0; i < n; ++i)
    {
        slots_[static_cast<size_t>(i)]->setIndices(section_, measure_, i);
        slots_[static_cast<size_t>(i)]->onSelected = [this, i] {
            if (onSlotSelected)
                onSlotSelected(i);
        };
        slots_[static_cast<size_t>(i)]->onAudition = [this, i] {
            if (onSlotAudition)
                onSlotAudition(i);
        };
    }

    setPlayingSlot(playingSlot_, playProgress_);
    resized();
    repaint();
}

void MeasureComponent::handleResize(int slotIndex, bool fromLeft, int x)
{
    const int newSpan = spanFromResizeX(slotIndex, fromLeft, x);
    if (newSpan == song_.slotSpan(section_, measure_, slotIndex))
        return;
    song_.resizeSlot(section_, measure_, slotIndex, newSpan, fromLeft);
    syncSlots();
}

int MeasureComponent::spanFromResizeX(int slotIndex, bool fromLeft, int x) const
{
    if (section_ < 0 || section_ >= static_cast<int>(song_.sections().size()))
        return 1;
    const auto& section = song_.sections()[static_cast<size_t>(section_)];
    if (measure_ < 0 || measure_ >= static_cast<int>(section.measures.size()))
        return 1;

    const auto& slots = section.measures[static_cast<size_t>(measure_)].slots;
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size()))
        return 1;

    int start = 0;
    for (int i = 0; i < slotIndex; ++i)
        start += std::max(1, slots[static_cast<size_t>(i)].span);

    const int span = std::max(1, slots[static_cast<size_t>(slotIndex)].span);
    const int capacity = std::max(1, section.timeSig.maxSlots());
    const float unitW = static_cast<float>(std::max(1, getWidth())) / static_cast<float>(capacity);
    const int unit = juce::jlimit(0, capacity,
                                  (int) std::lround(static_cast<float>(x) / unitW));

    if (fromLeft)
        return (start + span) - unit;
    return unit - start;
}

void MeasureComponent::resized()
{
    auto r = getLocalBounds();
    if (slots_.empty())
        return;

    std::vector<int> spans;
    int totalSpan = 0;
    if (section_ >= 0 && section_ < static_cast<int>(song_.sections().size()))
    {
        const auto& section = song_.sections()[static_cast<size_t>(section_)];
        if (measure_ >= 0 && measure_ < static_cast<int>(section.measures.size()))
        {
            for (const auto& slot : section.measures[static_cast<size_t>(measure_)].slots)
            {
                const int span = std::max(1, slot.span);
                spans.push_back(span);
                totalSpan += span;
            }
        }
    }

    while (spans.size() < slots_.size())
    {
        spans.push_back(1);
        ++totalSpan;
    }
    if (totalSpan < 1)
        totalSpan = 1;

    int acc = 0;
    const int n = static_cast<int>(slots_.size());
    for (int i = 0; i < n; ++i)
    {
        const int nextAcc = acc + spans[static_cast<size_t>(i)];
        const int x0 = acc * r.getWidth() / totalSpan;
        const int x1 = (i == n - 1) ? r.getWidth() : nextAcc * r.getWidth() / totalSpan;
        slots_[static_cast<size_t>(i)]->setBounds(r.getX() + x0, r.getY(),
                                                  juce::jmax(8, x1 - x0), r.getHeight());
        acc = nextAcc;
    }
}

} // namespace chords
