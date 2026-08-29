#pragma once

#include "gui/SectionComponent.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace chords
{

class SectionListComponent : public juce::Component
{
public:
    explicit SectionListComponent(Song& song);

    void rebuild();
    void setPlayhead(int section, int measure, int slot, bool playing, float progress = 0.0f);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    std::function<void(int section, int measure, int slot)> onSlotSelected;
    std::function<void(int section, int measure, int slot)> onSlotAudition;

private:
    class Content : public juce::Component
    {
    public:
        void paint(juce::Graphics&) override {}
        void resized() override {}
    };

    Song& song_;
    juce::Viewport viewport_;
    Content content_;
    juce::TextButton addSection_ { "+ Append Section" };
    std::vector<std::unique_ptr<SectionComponent>> sections_;
    int playSection_ = -1;
    int playMeasure_ = -1;
    int playSlot_ = -1;
    bool playing_ = false;
    float playProgress_ = 0.0f;
};

} // namespace chords
