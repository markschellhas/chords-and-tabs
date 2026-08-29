#pragma once

#include "gui/MeasureComponent.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace chords
{

class TimeSigView : public juce::Component
{
public:
    void set(TimeSignature ts) { ts_ = ts; repaint(); }
    void paint(juce::Graphics& g) override;

private:
    TimeSignature ts_ { 4, 4 };
};

class SectionComponent : public juce::Component
{
public:
    SectionComponent(Song& song, int sectionIndex);

    void rebuild();
    void setPlayhead(int measure, int slot);
    int preferredHeight(int width) const;
    static constexpr int kHeaderH = 34;
    static constexpr int kRowH = MeasureComponent::kHeight + 4;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int measure, int slot)> onSlotSelected;

private:
    void layoutMeasures(juce::Rectangle<int> area, bool apply) const;

    Song& song_;
    int sectionIndex_ = 0;
    int playingMeasure_ = -1;
    int playingSlot_ = -1;

    juce::Label name_;
    juce::ComboBox timeSig_;
    juce::TextButton addMeasure_ { "+ bar" };
    juce::TextButton remove_ { "X" };
    TimeSigView timeSigView_;
    std::vector<std::unique_ptr<MeasureComponent>> measures_;
};

} // namespace chords
