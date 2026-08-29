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

PianoKeyboard::PianoKeyboard()
{
    setOpaque(true);
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

    auto area = getLocalBounds().toFloat();
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
