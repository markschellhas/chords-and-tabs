#include "gui/PianoKeyboard.h"

namespace chords
{
namespace
{
bool isBlack(int midi)
{
    switch (midi % 12)
    {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}
} // namespace

ChevronButton::ChevronButton(bool pointLeft)
    : juce::Button(pointLeft ? "prev-sound" : "next-sound"),
      left_(pointLeft)
{
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(false);
    setTooltip(pointLeft ? "Previous sound" : "Next sound");
}

void ChevronButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    auto colour = look != nullptr ? look->text() : juce::Colours::white;
    if (down)
        colour = look != nullptr ? look->accent() : colour.brighter(0.2f);
    else if (highlighted)
        colour = colour.brighter(0.15f);
    else if (look != nullptr)
        colour = look->muted();

    auto r = getLocalBounds().toFloat().reduced(5.0f, 6.0f);
    const float cx = r.getCentreX();
    const float cy = r.getCentreY();
    const float dx = left_ ? -4.2f : 4.2f;

    juce::Path chevron;
    chevron.startNewSubPath(cx - dx, cy - 5.4f);
    chevron.lineTo(cx + dx, cy);
    chevron.lineTo(cx - dx, cy + 5.4f);

    g.setColour(colour);
    g.strokePath(chevron, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

SoundPicker::SoundPicker()
{
    name_.setJustificationType(juce::Justification::centred);
    name_.setInterceptsMouseClicks(false, false);
    name_.setMinimumHorizontalScale(0.7f);
    name_.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));

    prev_.onClick = [this] {
        if (onCycle)
            onCycle(-1);
    };
    next_.onClick = [this] {
        if (onCycle)
            onCycle(1);
    };

    addAndMakeVisible(prev_);
    addAndMakeVisible(next_);
    addAndMakeVisible(name_);
}

void SoundPicker::setName(const juce::String& name)
{
    name_.setText(name, juce::dontSendNotification);
}

void SoundPicker::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto panel = look != nullptr ? look->panel() : juce::Colour(0xff1c1f2a);
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;

    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(panel);
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(muted.withAlpha(0.4f));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);
}

void SoundPicker::resized()
{
    auto r = getLocalBounds().reduced(2, 1);
    prev_.setBounds(r.removeFromLeft(26));
    next_.setBounds(r.removeFromRight(26));
    name_.setBounds(r);
}

PianoKeyboard::PianoKeyboard()
{
    setOpaque(true);
    picker_.onCycle = [this](int delta) {
        if (onCycleSound)
            onCycleSound(delta);
    };
    addAndMakeVisible(picker_);
}

void PianoKeyboard::setSoundName(const juce::String& name)
{
    picker_.setName(name);
}

void PianoKeyboard::setHighlightedNotes(const std::array<int, 3>& notes)
{
    std::set<int> next;
    for (int n : notes)
        if (n >= 0)
            next.insert(n);
    if (next == highlighted_)
        return;
    highlighted_ = std::move(next);
    repaint();
}

void PianoKeyboard::clearHighlights()
{
    if (highlighted_.empty())
        return;
    highlighted_.clear();
    repaint();
}

bool PianoKeyboard::isHighlighted(int midi) const
{
    return highlighted_.count(midi) > 0;
}

void PianoKeyboard::resized()
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(kPickerHeight);
    const int pickerW = juce::jmin(214, juce::jmax(160, header.getWidth() / 3));
    picker_.setBounds(header.withSizeKeepingCentre(pickerW, kPickerHeight - 2));
    keyArea_ = bounds.toFloat();
    rebuildKeys();
}

void PianoKeyboard::rebuildKeys()
{
    whites_.clear();
    blacks_.clear();

    int whiteCount = 0;
    for (int m = kLowest; m <= kHighest; ++m)
        if (! isBlack(m))
            ++whiteCount;

    if (whiteCount <= 0)
        return;

    auto area = keyArea_;
    if (area.isEmpty())
        area = getLocalBounds().toFloat();
    const float whiteW = area.getWidth() / static_cast<float>(whiteCount);
    const float whiteH = area.getHeight();
    const float blackW = whiteW * 0.58f;
    const float blackH = whiteH * 0.62f;

    float x = area.getX();
    for (int m = kLowest; m <= kHighest; ++m)
    {
        if (isBlack(m))
            continue;
        Key k { m, false, { x, area.getY(), whiteW, whiteH } };
        whites_.push_back(k);
        x += whiteW;
    }

    for (size_t i = 0; i + 1 < whites_.size(); ++i)
    {
        const int midi = whites_[i].midi;
        const int nextBlack = midi + 1;
        if (nextBlack <= kHighest && isBlack(nextBlack) && nextBlack != whites_[i + 1].midi)
        {
            const float cx = whites_[i].bounds.getRight();
            blacks_.push_back({ nextBlack, true,
                { cx - blackW * 0.5f, area.getY(), blackW, blackH } });
        }
    }
}

void PianoKeyboard::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto bg = look != nullptr ? look->background() : juce::Colour(0xff12141a);
    const auto white = look != nullptr ? look->keyboardWhite() : juce::Colour(0xffe7e4dc);
    const auto black = look != nullptr ? look->keyboardBlack() : juce::Colour(0xff16181f);
    const auto hi = look != nullptr ? look->keyHighlight() : juce::Colour(0xff7eb6e8);
    const auto muted = look != nullptr ? look->muted() : juce::Colours::grey;

    g.fillAll(bg);

    for (const auto& k : whites_)
    {
        auto r = k.bounds.reduced(0.6f, 0.0f);
        g.setColour(isHighlighted(k.midi) ? hi : white);
        g.fillRoundedRectangle(r, 2.0f);
        g.setColour(bg);
        g.drawRoundedRectangle(r, 2.0f, 1.0f);

        if (k.midi % 12 == 0)
        {
            g.setColour(isHighlighted(k.midi) ? juce::Colour(0xff102030) : muted);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            const int octave = k.midi / 12 - 1;
            g.drawText("C" + juce::String(octave),
                       r.removeFromBottom(18.0f), juce::Justification::centred);
        }
    }

    for (const auto& k : blacks_)
    {
        auto r = k.bounds;
        g.setColour(isHighlighted(k.midi) ? hi.darker(0.15f) : black);
        g.fillRoundedRectangle(r, 2.0f);
        if (isHighlighted(k.midi))
        {
            g.setColour(hi.brighter(0.15f));
            g.drawRoundedRectangle(r, 2.0f, 1.2f);
        }
    }
}

} // namespace chords
