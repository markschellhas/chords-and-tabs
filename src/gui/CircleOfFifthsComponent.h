#pragma once

#include "gui/AppLookAndFeel.h"
#include "theory/MusicTheory.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace chords
{

class DiatonicChip : public juce::Component
{
public:
    void setChord(const Chord& chord, const juce::String& numeral);
    const Chord& getChord() const { return chord_; }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    Chord chord_;
    juce::String numeral_;
    bool dragging_ = false;
};

class CircleOfFifthsComponent : public juce::Component,
                                private juce::Timer
{
public:
    CircleOfFifthsComponent();

    int selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index);
    void rotate(int delta);

    std::function<void(int /*index*/)> onSelectionChanged;
    std::function<void(const Chord&)> onChordPreview;
    std::function<void()> onPreviewEnd;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    struct Wedge
    {
        juce::Path path;
        int index = 0;
        bool inner = false;
        Chord chord;
        float midAngle = 0.0f;
        float r0 = 0.0f;
        float r1 = 0.0f;
        bool visible = false;
    };

    void timerCallback() override;
    void rebuildWedges();
    void rebuildChips();
    juce::Rectangle<float> fanBounds() const;
    int hitWedge(juce::Point<float> p) const;
    void startChordDrag(const Chord& chord, juce::Component* source);
    AppLookAndFeel* laf() const;

    int selectedIndex_ = 0;
    float currentRotation_ = 0.0f;
    int pressedWedge_ = -1;
    bool dragging_ = false;
    std::vector<Wedge> wedges_;
    juce::Point<float> centre_;
    float outerRadius_ = 1.0f;
    float innerInner_ = 0.28f;
    float innerOuter_ = 0.62f;
    float outerInner_ = 0.66f;
    std::array<DiatonicChip, 7> chips_;
};

} // namespace chords
