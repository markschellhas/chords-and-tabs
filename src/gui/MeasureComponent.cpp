#include "gui/MeasureComponent.h"

namespace chords
{

MeasureComponent::MeasureComponent(Song& song, int section, int measure)
    : song_(song), section_(section), measure_(measure)
{
    addSlot_.setButtonText("+");
    addSlot_.setTooltip("Add another chord in this bar");
    addSlot_.onClick = [this] { song_.addSlot(section_, measure_); };
    addAndMakeVisible(addSlot_);

    remove_.setButtonText("-");
    remove_.setTooltip("Remove this bar");
    remove_.onClick = [this] { song_.removeMeasure(section_, measure_); };
    addAndMakeVisible(remove_);

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

int MeasureComponent::preferredWidth() const
{
    const int n = juce::jmax(1, static_cast<int>(slots_.size()));
    return 16 + n * 76 + 44;
}

void MeasureComponent::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(muted.withAlpha(0.55f));
    g.drawLine(r.getX() + 4.0f, r.getY() + 6.0f, r.getRight() - 22.0f, r.getY() + 6.0f, 1.2f);
}

void MeasureComponent::resized()
{
    auto r = getLocalBounds().reduced(4, 10);
    remove_.setBounds(r.removeFromRight(18).removeFromTop(18));
    addSlot_.setBounds(r.removeFromRight(18).removeFromTop(18));
    r.removeFromRight(2);
    const int n = juce::jmax(1, static_cast<int>(slots_.size()));
    const int w = juce::jmax(36, r.getWidth() / n);
    for (auto& slot : slots_)
        slot->setBounds(r.removeFromLeft(w).reduced(1, 0));
}

} // namespace chords
