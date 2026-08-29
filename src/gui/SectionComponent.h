#pragma once

#include "gui/MeasureComponent.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace chords
{

class MoreButton : public juce::Button
{
public:
    MoreButton();

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override;
};

class SectionComponent : public juce::Component
{
public:
    SectionComponent(Song& song, int sectionIndex);

    void rebuild();
    void setPlayhead(int measure, int slot, float progress = 0.0f);
    int preferredHeight(int width) const;
    static constexpr int kHeaderH = 36;
    static constexpr int kChordH = MeasureComponent::kHeight;
    static constexpr int kGap = 6;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    std::function<void(int measure, int slot)> onSlotSelected;
    std::function<void(int measure, int slot)> onSlotAudition;

private:
    void showMoreMenu();
    void showEditMenu();
    void applyTimeSignature(int menuId);
    void startRename();
    void layoutMeasures(juce::Rectangle<int> area, bool apply) const;
    int measureWidth(int areaWidth) const;

    Song& song_;
    int sectionIndex_ = 0;
    int playingMeasure_ = -1;
    int playingSlot_ = -1;
    float playProgress_ = 0.0f;

    MoreButton more_;
    juce::Label name_;
    juce::TextButton edit_ { "Edit" };
    std::vector<std::unique_ptr<MeasureComponent>> measures_;
};

} // namespace chords
