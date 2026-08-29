#include "gui/ChordSlotComponent.h"

namespace chords
{

ChordSlotComponent::ChordSlotComponent(Song& song, int section, int measure, int slot)
    : song_(song), section_(section), measure_(measure), slot_(slot)
{
}

void ChordSlotComponent::setPlaying(bool playing)
{
    if (playing_ == playing)
        return;
    playing_ = playing;
    repaint();
}

void ChordSlotComponent::setIndices(int section, int measure, int slot)
{
    section_ = section;
    measure_ = measure;
    slot_ = slot;
    repaint();
}

AppLookAndFeel* ChordSlotComponent::laf() const
{
    return dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
}

std::optional<Chord> ChordSlotComponent::chord() const
{
    return song_.getChord(section_, measure_, slot_);
}

bool ChordSlotComponent::hitClear(juce::Point<float> p) const
{
    auto r = getLocalBounds().toFloat();
    juce::Rectangle<float> x(r.getRight() - 16.0f, r.getY() + 2.0f, 14.0f, 14.0f);
    return chord().has_value() && x.contains(p);
}

ChordSlotComponent::Edge ChordSlotComponent::hitEdge(juce::Point<float> p) const
{
    if (! chord())
        return Edge::None;

    auto r = getLocalBounds().toFloat();
    const float grip = juce::jmin(10.0f, r.getWidth() * 0.25f);
    if (grip < 4.0f)
        return Edge::None;
    if (p.x <= r.getX() + grip)
        return Edge::Left;
    if (p.x >= r.getRight() - grip)
        return Edge::Right;
    return Edge::None;
}

void ChordSlotComponent::updateCursor(juce::Point<float> p)
{
    if (hitClear(p))
        setMouseCursor(juce::MouseCursor::NormalCursor);
    else if (hitEdge(p) != Edge::None)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ChordSlotComponent::rememberIncoming(const SourceDetails& details)
{
    if (auto c = decodeChord(details.description.toString().toStdString()))
        incomingName_ = c->name();
    else
        incomingName_.clear();
    splitAfter_ = details.localPosition.x >= getWidth() * 0.5f;
}

void ChordSlotComponent::paint(juce::Graphics& g)
{
    auto* look = laf();
    const auto chip = look != nullptr ? look->chip() : juce::Colour(0xff3a4768);
    const auto chipText = look != nullptr ? look->chipText() : juce::Colours::white;
    const auto accent = look != nullptr ? look->accent() : juce::Colour(0xff7eb6e8);
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;

    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const auto c = chord();
    const juce::String name = c ? juce::String(c->name()) : juce::String();
    const bool splitting = dropHover_ && c.has_value() && song_.canSplitSlot(section_, measure_, slot_);

    if (splitting)
    {
        auto left = bounds;
        auto right = left.removeFromRight(left.getWidth() * 0.5f);
        left.removeFromRight(2.0f);
        const auto existingR = splitAfter_ ? left : right;
        const auto incomingR = splitAfter_ ? right : left;

        AppLookAndFeel::drawChordChip(g, existingR, name, true, playing_, false,
                                      chip, chipText, accent, muted);
        AppLookAndFeel::drawChordChip(g, incomingR, incomingName_, true, false, true,
                                      chip, chipText, accent, muted);
    }
    else
    {
        AppLookAndFeel::drawChordChip(g, bounds, name, c.has_value(), playing_, dropHover_,
                                      chip, chipText, accent, muted);
    }

    if (c.has_value() && hover_ && ! dropHover_)
    {
        g.setColour(accent.withAlpha(0.65f));
        const float gh = juce::jmax(8.0f, bounds.getHeight() - 16.0f);
        juce::Rectangle<float> leftGrip(bounds.getX(), bounds.getCentreY() - gh * 0.5f, 3.0f, gh);
        juce::Rectangle<float> rightGrip(bounds.getRight() - 3.0f, bounds.getCentreY() - gh * 0.5f, 3.0f, gh);
        g.fillRoundedRectangle(leftGrip, 1.5f);
        g.fillRoundedRectangle(rightGrip, 1.5f);

        auto x = juce::Rectangle<float>(bounds.getRight() - 14.0f, bounds.getY() + 3.0f, 11.0f, 11.0f);
        g.setColour(muted.withAlpha(0.9f));
        g.drawLine(x.getX(), x.getY(), x.getRight(), x.getBottom(), 1.2f);
        g.drawLine(x.getRight(), x.getY(), x.getX(), x.getBottom(), 1.2f);
    }
}

void ChordSlotComponent::mouseEnter(const juce::MouseEvent& e)
{
    hover_ = true;
    updateCursor(e.position);
    repaint();
}

void ChordSlotComponent::mouseExit(const juce::MouseEvent&)
{
    hover_ = false;
    hoverClear_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void ChordSlotComponent::mouseMove(const juce::MouseEvent& e)
{
    updateCursor(e.position);
}

void ChordSlotComponent::mouseDown(const juce::MouseEvent& e)
{
    dragging_ = false;
    resizing_ = false;
    hoverClear_ = hitClear(e.position);
    if (! hoverClear_)
    {
        const auto edge = hitEdge(e.position);
        if (edge != Edge::None)
        {
            resizing_ = true;
            resizeFromLeft_ = edge == Edge::Left;
        }
    }
    if (onSelected)
        onSelected();
}

void ChordSlotComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (hoverClear_)
        return;

    if (resizing_)
    {
        if (e.getDistanceFromDragStart() < 2)
            return;
        if (! gesture_)
        {
            song_.beginGesture();
            gesture_ = true;
        }
        if (onResizeDrag != nullptr && getParentComponent() != nullptr)
        {
            const auto parentPos = getParentComponent()->getLocalPoint(this, e.position);
            onResizeDrag(slot_, resizeFromLeft_, parentPos.x);
        }
        return;
    }

    if (dragging_ || e.getDistanceFromDragStart() < 6)
        return;
    const auto c = chord();
    if (! c)
        return;
    dragging_ = true;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::Image img(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(img);
        paint(g);
        container->startDragging(juce::var(juce::String(encodeChord(*c))), this, juce::ScaledImage(img));
    }
}

void ChordSlotComponent::mouseUp(const juce::MouseEvent& e)
{
    if (gesture_)
    {
        song_.endGesture();
        gesture_ = false;
    }
    if (resizing_)
    {
        resizing_ = false;
        return;
    }
    if (! dragging_ && hoverClear_ && hitClear(e.position))
        song_.setChord(section_, measure_, slot_, std::nullopt);
    dragging_ = false;
    hoverClear_ = false;
}

bool ChordSlotComponent::isInterestedInDragSource(const SourceDetails& details)
{
    return decodeChord(details.description.toString().toStdString()).has_value();
}

void ChordSlotComponent::itemDragEnter(const SourceDetails& details)
{
    dropHover_ = true;
    rememberIncoming(details);
    repaint();
}

void ChordSlotComponent::itemDragMove(const SourceDetails& details)
{
    rememberIncoming(details);
    repaint();
}

void ChordSlotComponent::itemDragExit(const SourceDetails&)
{
    dropHover_ = false;
    incomingName_.clear();
    repaint();
}

void ChordSlotComponent::itemDropped(const SourceDetails& details)
{
    dropHover_ = false;
    incomingName_.clear();
    if (details.sourceComponent.get() == this)
    {
        repaint();
        return;
    }
    if (auto c = decodeChord(details.description.toString().toStdString()))
    {
        const bool insertAfter = details.localPosition.x >= getWidth() * 0.5f;
        song_.placeChord(section_, measure_, slot_, *c, insertAfter);
    }
    else
    {
        repaint();
    }
}

} // namespace chords
