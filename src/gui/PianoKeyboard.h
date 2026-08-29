#pragma once

#include "gui/AppLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <map>
#include <set>
#include <vector>

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

class KeyboardToggle : public juce::Button
{
public:
    KeyboardToggle();

    void paintButton(juce::Graphics& g,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;
};

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard();
    ~PianoKeyboard() override;

    void setHighlightedNotes(const std::array<int, 3>& notes);
    void clearHighlights();
    void setSoundName(const juce::String& name);

    bool isComputerKeyboardEnabled() const { return computerKeyboardOn_; }
    bool handleComputerKeyPress(const juce::KeyPress& key);
    bool syncComputerKeyState();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void(int delta)> onCycleSound;
    std::function<void(int midi)> onNoteOn;
    std::function<void(int midi)> onNoteOff;
    std::function<void(bool enabled)> onComputerKeyboardToggled;

private:
    struct Key
    {
        int midi = 0;
        bool black = false;
        juce::Rectangle<float> bounds;
    };

    void rebuildKeys();
    bool isHighlighted(int midi) const;
    int midiAt(juce::Point<float> pos) const;
    void beginInteractiveNote(int midi);
    void endInteractiveNote(int midi);
    void setMouseNote(int midi);
    void releaseComputerNotes();
    void releaseAllInteractiveNotes();
    void setComputerKeyboardEnabled(bool enabled);
    void shiftOctave(int delta);
    static char charFromKeyPress(const juce::KeyPress& key);
    static bool isPhysicalKeyDown(char key);

    static constexpr int kLowest = 48;  // C3
    static constexpr int kHighest = 72; // C5
    static constexpr int kPickerHeight = 28;

    SoundPicker picker_;
    KeyboardToggle keyboardToggle_;
    juce::Rectangle<float> keyArea_;
    std::vector<Key> whites_;
    std::vector<Key> blacks_;
    std::set<int> highlighted_;
    std::set<int> pressed_;
    std::map<int, int> pressCount_;
    std::map<char, int> computerNotes_;
    std::set<char> octaveHeld_;
    int mouseNote_ = -1;
    int octave_ = 4;
    bool computerKeyboardOn_ = false;
};

} // namespace chords
