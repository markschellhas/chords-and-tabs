#pragma once

#include "gui/AppLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <set>

namespace chords
{

class ChevronButton : public juce::Button
{
public:
    explicit ChevronButton(bool pointLeft);

    void paintButton(juce::Graphics& g,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

private:
    bool left_ = true;
};

class SoundPicker : public juce::Component
{
public:
    SoundPicker();

    void setName(const juce::String& name);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int delta)> onCycle;

private:
    ChevronButton prev_ { true };
    ChevronButton next_ { false };
    juce::Label name_;
};

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard();

    void setHighlightedNotes(const std::array<int, 3>& notes);
    void clearHighlights();
    void setSoundName(const juce::String& name);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int delta)> onCycleSound;

private:
    struct Key
    {
        int midi = 0;
        bool black = false;
        juce::Rectangle<float> bounds;
    };

    void rebuildKeys();
    bool isHighlighted(int midi) const;

    static constexpr int kLowest = 48;  // C3
    static constexpr int kHighest = 72; // C5
    static constexpr int kPickerHeight = 28;

    SoundPicker picker_;
    juce::Rectangle<float> keyArea_;
    std::vector<Key> whites_;
    std::vector<Key> blacks_;
    std::set<int> highlighted_;
};

} // namespace chords
