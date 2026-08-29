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

    AppLookAndFeel::drawChordChip(g, bounds, name, c.has_value(), playing_, dropHover_,
                                  chip, chipText, accent, muted);

    if (c.has_value() && hover_)
    {
        auto x = juce::Rectangle<float>(bounds.getRight() - 14.0f, bounds.getY() + 3.0f, 11.0f, 11.0f);
        g.setColour(muted.withAlpha(0.9f));
        g.drawLine(x.getX(), x.getY(), x.getRight(), x.getBottom(), 1.2f);
        g.drawLine(x.getRight(), x.getY(), x.getX(), x.getBottom(), 1.2f);
    }
}

void ChordSlotComponent::mouseEnter(const juce::MouseEvent&)
{
    hover_ = true;
    repaint();
}

void ChordSlotComponent::mouseExit(const juce::MouseEvent&)
{
    hover_ = false;
    hoverClear_ = false;
    repaint();
}

void ChordSlotComponent::mouseDown(const juce::MouseEvent& e)
{
    dragging_ = false;
    hoverClear_ = hitClear(e.position);
    if (onSelected)
        onSelected();
}

void ChordSlotComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (hoverClear_ || dragging_ || e.getDistanceFromDragStart() < 6)
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
    if (! dragging_ && hoverClear_ && hitClear(e.position))
        song_.setChord(section_, measure_, slot_, std::nullopt);
    dragging_ = false;
    hoverClear_ = false;
}

bool ChordSlotComponent::isInterestedInDragSource(const SourceDetails& details)
{
    return decodeChord(details.description.toString().toStdString()).has_value();
}

void ChordSlotComponent::itemDragEnter(const SourceDetails&)
{
    dropHover_ = true;
    repaint();
}

void ChordSlotComponent::itemDragExit(const SourceDetails&)
{
    dropHover_ = false;
    repaint();
}

void ChordSlotComponent::itemDropped(const SourceDetails& details)
{
    dropHover_ = false;
    if (auto c = decodeChord(details.description.toString().toStdString()))
        song_.setChord(section_, measure_, slot_, *c);
    else
        repaint();
}

} // namespace chords
