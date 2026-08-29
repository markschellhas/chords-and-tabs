#pragma once

#include "gui/AppLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <set>

namespace chords
{

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard();

    void setHighlightedNotes(const std::array<int, 3>& notes);
    void clearHighlights();

    void paint(juce::Graphics& g) override;
    void resized() override;

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

    std::vector<Key> whites_;
    std::vector<Key> blacks_;
    std::set<int> highlighted_;
};

} // namespace chords
