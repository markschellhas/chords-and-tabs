#include "gui/CircleOfFifthsComponent.h"

#include <cmath>

namespace chords
{

namespace
{
constexpr float kChipStripH = 56.0f;
constexpr float kFanGapH = 6.0f;
// Modest zoom of the original semicircle so the clip still shows the
// active key's six wedges (I / IV / V and their relative minors) plus
// a sliver of the next chords on each side.
constexpr float kCircleZoom = 1.5f;
constexpr float kInnerInnerT = 0.28f;
constexpr float kInnerOuterT = 0.62f;
constexpr float kOuterInnerT = 0.66f;
}

void DiatonicChip::setChord(const Chord& chord, const juce::String& numeral)
{
    chord_ = chord;
    numeral_ = numeral;
    repaint();
}

void DiatonicChip::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto chip = look != nullptr ? look->chip() : juce::Colour(0xff3a4768);
    const auto text = look != nullptr ? look->chipText() : juce::Colours::white;
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;

    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(chip);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(chip.brighter(0.22f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(numeral_, bounds.removeFromTop(13.0f), juce::Justification::centred);

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    g.drawText(chord_.name(), bounds, juce::Justification::centred);
}

void DiatonicChip::mouseDown(const juce::MouseEvent&)
{
    dragging_ = false;
}

void DiatonicChip::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging_ || e.getDistanceFromDragStart() < 6)
        return;
    dragging_ = true;
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        juce::Image img(juce::Image::ARGB, 72, 36, true);
        juce::Graphics g(img);
        g.setColour(juce::Colour(0xff3a4768));
        g.fillRoundedRectangle(0.0f, 0.0f, 72.0f, 36.0f, 6.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
        g.drawText(chord_.name(), img.getBounds().toFloat(), juce::Justification::centred);
        container->startDragging(juce::var(juce::String(encodeChord(chord_))), this, juce::ScaledImage(img));
    }
}

CircleOfFifthsComponent::CircleOfFifthsComponent()
{
    for (auto& chip : chips_)
        addAndMakeVisible(chip);
    rebuildChips();
    startTimerHz(60);
    setOpaque(true);
}

AppLookAndFeel* CircleOfFifthsComponent::laf() const
{
    return dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
}

void CircleOfFifthsComponent::setSelectedIndex(int index)
{
    index = CircleOfFifths::wrap(index);
    if (index == selectedIndex_)
        return;
    selectedIndex_ = index;
    rebuildChips();
    if (onSelectionChanged)
        onSelectionChanged(selectedIndex_);
    repaint();
}

void CircleOfFifthsComponent::rotate(int delta)
{
    setSelectedIndex(selectedIndex_ + delta);
}

void CircleOfFifthsComponent::rebuildChips()
{
    const auto triads = CircleOfFifths::diatonicTriads(selectedIndex_);
    const auto numerals = CircleOfFifths::diatonicNumerals();
    for (int i = 0; i < 7; ++i)
        chips_[static_cast<size_t>(i)].setChord(triads[static_cast<size_t>(i)], numerals[static_cast<size_t>(i)]);
}

juce::Rectangle<float> CircleOfFifthsComponent::fanBounds() const
{
    auto fan = getLocalBounds().toFloat();
    fan.removeFromBottom(kChipStripH);
    return fan;
}

void CircleOfFifthsComponent::resized()
{
    auto r = getLocalBounds();
    auto strip = r.removeFromBottom(static_cast<int>(kChipStripH)).reduced(12, 8);
    const int chipW = juce::jmax(48, strip.getWidth() / 7);
    for (int i = 0; i < 7; ++i)
        chips_[static_cast<size_t>(i)].setBounds(strip.removeFromLeft(chipW).reduced(3, 2));
    rebuildWedges();
}

void CircleOfFifthsComponent::rebuildWedges()
{
    wedges_.clear();
    auto fan = fanBounds();
    fan.removeFromBottom(kFanGapH);
    if (fan.getWidth() < 8.0f || fan.getHeight() < 8.0f)
        return;

    const float baseRadius = juce::jmin(fan.getWidth() * 0.45f, fan.getHeight() * 0.98f);
    outerRadius_ = baseRadius * kCircleZoom;

    // Drop the centre so the 12 o'clock rim stays in the same-height box.
    const float rimInset = juce::jmax(4.0f, fan.getHeight() * 0.03f);
    centre_ = { fan.getCentreX(), fan.getY() + rimInset + outerRadius_ };

    innerInner_ = outerRadius_ * kInnerInnerT;
    innerOuter_ = outerRadius_ * kInnerOuterT;
    outerInner_ = outerRadius_ * kOuterInnerT;

    const float step = juce::MathConstants<float>::twoPi / static_cast<float>(CircleOfFifths::kCount);

    auto addRing = [&](float r0, float r1, bool inner) {
        for (int i = 0; i < CircleOfFifths::kCount; ++i)
        {
            const float a0 = static_cast<float>(i) * step - step * 0.5f - currentRotation_;
            const float a1 = a0 + step;
            float mid = 0.5f * (a0 + a1);
            while (mid > juce::MathConstants<float>::pi)
                mid -= juce::MathConstants<float>::twoPi;
            while (mid < -juce::MathConstants<float>::pi)
                mid += juce::MathConstants<float>::twoPi;

            Wedge w;
            w.index = i;
            w.inner = inner;
            w.chord = inner ? CircleOfFifths::minorChord(i) : CircleOfFifths::majorChord(i);
            w.midAngle = mid;
            w.r0 = r0;
            w.r1 = r1;

            w.path.addCentredArc(centre_.x, centre_.y, r1, r1, 0.0f, a0, a1, true);
            w.path.addCentredArc(centre_.x, centre_.y, r0, r0, 0.0f, a1, a0, false);
            w.path.closeSubPath();
            w.visible = w.path.getBounds().intersects(fan);
            wedges_.push_back(std::move(w));
        }
    };

    addRing(outerInner_, outerRadius_, false);
    addRing(innerInner_, innerOuter_, true);
}

void CircleOfFifthsComponent::timerCallback()
{
    const float step = juce::MathConstants<float>::twoPi / static_cast<float>(CircleOfFifths::kCount);
    float target = static_cast<float>(selectedIndex_) * step;
    float delta = target - currentRotation_;
    while (delta > juce::MathConstants<float>::pi)
        delta -= juce::MathConstants<float>::twoPi;
    while (delta < -juce::MathConstants<float>::pi)
        delta += juce::MathConstants<float>::twoPi;

    if (std::abs(delta) < 0.002f)
    {
        if (std::abs(currentRotation_ - target) > 1.0e-5f)
        {
            currentRotation_ = target;
            rebuildWedges();
            repaint();
        }
        return;
    }

    currentRotation_ += delta * 0.22f;
    rebuildWedges();
    repaint();
}

void CircleOfFifthsComponent::paint(juce::Graphics& g)
{
    auto* look = laf();
    const auto bg = look != nullptr ? look->circleBackground() : juce::Colour(0xff0d1018);
    const auto wedgeCol = look != nullptr ? look->wedge() : juce::Colour(0xff232838);
    const auto active = look != nullptr ? look->wedgeActive() : juce::Colour(0xff6a7bb5);
    const auto innerActive = look != nullptr ? look->wedgeInnerActive() : juce::Colour(0xff8b97d0);
    const auto text = look != nullptr ? look->text() : juce::Colours::whitesmoke;
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;

    g.fillAll(bg);

    auto fan = fanBounds();
    g.saveState();
    g.reduceClipRegion(fan.toNearestInt());

    for (const auto& w : wedges_)
    {
        if (! w.visible)
            continue;

        const bool selected = w.index == selectedIndex_;
        const float fade = juce::jlimit(0.35f, 1.0f, 1.0f - std::abs(w.midAngle) * 0.5f);

        juce::Colour fill = wedgeCol;
        if (selected)
            fill = w.inner ? innerActive : active;
        g.setColour(fill.withMultipliedAlpha(fade));
        g.fillPath(w.path);

        g.setColour(bg.withAlpha(0.85f));
        g.strokePath(w.path, juce::PathStrokeType(1.4f));

        const float midR = 0.5f * (w.r0 + w.r1);
        const auto pos = juce::Point<float>(centre_.x + std::sin(w.midAngle) * midR,
                                            centre_.y - std::cos(w.midAngle) * midR);
        if (! fan.reduced(4.0f, 2.0f).contains(pos))
            continue;

        g.setColour((selected ? juce::Colours::white : text).withMultipliedAlpha(fade));
        const float fontH = w.inner ? 14.0f : 18.0f;
        g.setFont(juce::Font(juce::FontOptions(fontH, selected ? juce::Font::bold : juce::Font::plain)));
        g.drawText(w.chord.name(),
                   juce::Rectangle<float>(pos.x - 26.0f, pos.y - 11.0f, 52.0f, 22.0f),
                   juce::Justification::centred);
    }

    g.restoreState();
    juce::ignoreUnused(muted);
}

int CircleOfFifthsComponent::hitWedge(juce::Point<float> p) const
{
    for (int i = 0; i < static_cast<int>(wedges_.size()); ++i)
        if (wedges_[static_cast<size_t>(i)].visible && wedges_[static_cast<size_t>(i)].path.contains(p))
            return i;
    return -1;
}

void CircleOfFifthsComponent::startChordDrag(const Chord& chord, juce::Component* source)
{
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(source))
    {
        juce::Image img(juce::Image::ARGB, 80, 40, true);
        juce::Graphics g(img);
        g.setColour(juce::Colour(0xff3a4768));
        g.fillRoundedRectangle(0.0f, 0.0f, 80.0f, 40.0f, 7.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        g.drawText(chord.name(), img.getBounds().toFloat(), juce::Justification::centred);
        container->startDragging(juce::var(juce::String(encodeChord(chord))), source, juce::ScaledImage(img));
    }
}

void CircleOfFifthsComponent::mouseDown(const juce::MouseEvent& e)
{
    pressedWedge_ = hitWedge(e.position);
    dragging_ = false;
}

void CircleOfFifthsComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (pressedWedge_ < 0 || dragging_ || e.getDistanceFromDragStart() < 8)
        return;
    dragging_ = true;
    startChordDrag(wedges_[static_cast<size_t>(pressedWedge_)].chord, this);
}

void CircleOfFifthsComponent::mouseUp(const juce::MouseEvent& e)
{
    if (! dragging_ && pressedWedge_ >= 0)
    {
        const auto& w = wedges_[static_cast<size_t>(pressedWedge_)];
        if (w.path.contains(e.position))
            setSelectedIndex(w.index);
    }
    pressedWedge_ = -1;
    dragging_ = false;
}

void CircleOfFifthsComponent::mouseMove(const juce::MouseEvent& e)
{
    const int i = hitWedge(e.position);
    if (i >= 0)
    {
        if (onChordPreview)
            onChordPreview(wedges_[static_cast<size_t>(i)].chord);
    }
    else if (onPreviewEnd)
    {
        onPreviewEnd();
    }
}

void CircleOfFifthsComponent::mouseExit(const juce::MouseEvent&)
{
    if (onPreviewEnd)
        onPreviewEnd();
}

void CircleOfFifthsComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY > 0.02f || wheel.deltaX > 0.02f)
        rotate(-1);
    else if (wheel.deltaY < -0.02f || wheel.deltaX < -0.02f)
        rotate(1);
}

} // namespace chords
