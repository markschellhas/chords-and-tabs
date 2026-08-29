#pragma once

#include "gui/AppLookAndFeel.h"
#include "model/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace chords
{

class ChordSlotComponent : public juce::Component,
                           public juce::DragAndDropTarget
{
public:
    ChordSlotComponent(Song& song, int section, int measure, int slot);

    void setPlaying(bool playing);
    void setIndices(int section, int measure, int slot);

    void paint(juce::Graphics& g) override;
    void resized() override {}
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

    std::function<void()> onSelected;
    std::function<void(int slot, bool fromLeft, int parentX)> onResizeDrag;

private:
    enum class Edge
    {
        None,
        Left,
        Right
    };

    std::optional<Chord> chord() const;
    AppLookAndFeel* laf() const;
    bool hitClear(juce::Point<float> p) const;
    Edge hitEdge(juce::Point<float> p) const;
    void updateCursor(juce::Point<float> p);
    void rememberIncoming(const SourceDetails& details);

    Song& song_;
    int section_ = 0;
    int measure_ = 0;
    int slot_ = 0;
    bool playing_ = false;
    bool dropHover_ = false;
    bool dragging_ = false;
    bool hoverClear_ = false;
    bool hover_ = false;
    bool resizing_ = false;
    bool resizeFromLeft_ = false;
    bool gesture_ = false;
    bool splitAfter_ = true;
    juce::String incomingName_;
};

} // namespace chords
