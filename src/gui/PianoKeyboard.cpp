#include "gui/PianoKeyboard.h"
#include "theory/LaptopKeys.h"

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

void drawKeyRect(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour)
{
    g.setColour(colour);
    g.fillRoundedRectangle(r, 0.7f);
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

KeyboardToggle::KeyboardToggle()
    : juce::Button("computer-keyboard")
{
    setClickingTogglesState(true);
    setToggleState(false, juce::dontSendNotification);
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(false);
    setTooltip("Play notes from the laptop keyboard");
}

void KeyboardToggle::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const bool on = getToggleState();
    auto colour = look != nullptr ? look->muted() : juce::Colours::grey;
    if (on)
        colour = look != nullptr ? look->accent() : juce::Colour(0xff7eb6e8);
    else if (down)
        colour = look != nullptr ? look->text() : juce::Colours::white;
    else if (highlighted)
        colour = colour.brighter(0.2f);

    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    if (on)
    {
        g.setColour(colour.withAlpha(0.22f));
        g.fillRoundedRectangle(bounds, 6.0f);
    }
    else if (highlighted || down)
    {
        g.setColour(colour.withAlpha(0.12f));
        g.fillRoundedRectangle(bounds, 6.0f);
    }

    auto r = bounds.reduced(5.0f, 6.5f);
    g.setColour(colour);
    g.drawRoundedRectangle(r, 2.2f, 1.5f);

    auto inner = r.reduced(2.1f, 2.3f);
    const float rowH = inner.getHeight() / 3.15f;
    const float gap = 1.05f;

    const float topY = inner.getY();
    const float topW = (inner.getWidth() - gap * 4.0f) / 5.0f;
    for (int i = 0; i < 5; ++i)
        drawKeyRect(g, { inner.getX() + static_cast<float>(i) * (topW + gap), topY, topW, rowH * 0.78f }, colour);

    const float midY = topY + rowH + 0.6f;
    const float midInset = inner.getWidth() * 0.08f;
    const float midW = (inner.getWidth() - midInset - gap * 3.0f) / 4.0f;
    for (int i = 0; i < 4; ++i)
        drawKeyRect(g, { inner.getX() + midInset + static_cast<float>(i) * (midW + gap), midY, midW, rowH * 0.78f }, colour);

    const float barY = inner.getBottom() - rowH * 0.72f;
    const float barW = inner.getWidth() * 0.62f;
    drawKeyRect(g, { inner.getCentreX() - barW * 0.5f, barY, barW, rowH * 0.62f }, colour);
}

PianoKeyboard::PianoKeyboard()
{
    setOpaque(true);
    picker_.onCycle = [this](int delta) {
        if (onCycleSound)
            onCycleSound(delta);
    };
    keyboardToggle_.onClick = [this] {
        setComputerKeyboardEnabled(keyboardToggle_.getToggleState());
    };
    addAndMakeVisible(keyboardToggle_);
    addAndMakeVisible(picker_);
}

PianoKeyboard::~PianoKeyboard()
{
    releaseAllInteractiveNotes();
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
    if (highlighted_.count(midi) > 0 || pressed_.count(midi) > 0)
        return true;
    const auto it = lingerUntil_.find(midi);
    return it != lingerUntil_.end()
        && juce::Time::getMillisecondCounter() < it->second;
}

void PianoKeyboard::lingerNote(int midi)
{
    lingerUntil_[midi] = juce::Time::getMillisecondCounter() + 220;
    startTimerHz(30);
}

void PianoKeyboard::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();
    bool dirty = false;
    for (auto it = lingerUntil_.begin(); it != lingerUntil_.end();)
    {
        if (now >= it->second)
        {
            it = lingerUntil_.erase(it);
            dirty = true;
        }
        else
        {
            ++it;
        }
    }
    if (dirty)
        repaint();
    if (lingerUntil_.empty())
        stopTimer();
}

void PianoKeyboard::resized()
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(kPickerHeight);
    const int toggle = kPickerHeight - 2;
    keyboardToggle_.setBounds(header.removeFromLeft(toggle + 6).withSizeKeepingCentre(toggle, toggle));
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

int PianoKeyboard::midiAt(juce::Point<float> pos) const
{
    for (const auto& k : blacks_)
        if (k.bounds.contains(pos))
            return k.midi;
    for (const auto& k : whites_)
        if (k.bounds.contains(pos))
            return k.midi;
    return -1;
}

void PianoKeyboard::beginInteractiveNote(int midi)
{
    if (midi < 0)
        return;
    const int count = ++pressCount_[midi];
    pressed_.insert(midi);
    if (count == 1 && onNoteOn)
        onNoteOn(midi);
    repaint();
}

void PianoKeyboard::endInteractiveNote(int midi)
{
    if (midi < 0)
        return;
    const auto it = pressCount_.find(midi);
    if (it == pressCount_.end())
        return;
    if (--it->second > 0)
        return;
    pressCount_.erase(it);
    pressed_.erase(midi);
    lingerNote(midi);
    if (onNoteOff)
        onNoteOff(midi);
    repaint();
}

void PianoKeyboard::setMouseNote(int midi)
{
    if (midi == mouseNote_)
        return;
    if (mouseNote_ >= 0)
        endInteractiveNote(mouseNote_);
    mouseNote_ = midi;
    if (mouseNote_ >= 0)
        beginInteractiveNote(mouseNote_);
}

void PianoKeyboard::mouseDown(const juce::MouseEvent& e)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setMouseNote(midiAt(e.position));
}

void PianoKeyboard::mouseDrag(const juce::MouseEvent& e)
{
    if (mouseNote_ < 0 && ! e.mouseWasDraggedSinceMouseDown())
        return;
    setMouseNote(midiAt(e.position));
}

void PianoKeyboard::mouseUp(const juce::MouseEvent&)
{
    setMouseNote(-1);
}

void PianoKeyboard::setComputerKeyboardEnabled(bool enabled)
{
    if (computerKeyboardOn_ == enabled)
        return;

    computerKeyboardOn_ = enabled;
    keyboardToggle_.setToggleState(enabled, juce::dontSendNotification);
    keyboardToggle_.setTooltip(enabled
        ? "Laptop keyboard on — A=C, W=C♯, Z/X octave"
        : "Play notes from the laptop keyboard");

    if (! enabled)
        releaseComputerNotes();

    if (onComputerKeyboardToggled)
        onComputerKeyboardToggled(enabled);

    repaint();
}

void PianoKeyboard::shiftOctave(int delta)
{
    const int next = clampLaptopOctave(octave_ + delta);
    if (next == octave_)
        return;
    octave_ = next;
    if (computerKeyboardOn_)
        repaint();
}

void PianoKeyboard::releaseComputerNotes()
{
    for (const auto& [key, midi] : computerNotes_)
        endInteractiveNote(midi);
    computerNotes_.clear();
    octaveHeld_.clear();
}

void PianoKeyboard::releaseAllInteractiveNotes()
{
    setMouseNote(-1);
    releaseComputerNotes();
}

char PianoKeyboard::charFromKeyPress(const juce::KeyPress& key)
{
    const int code = key.getKeyCode();
    if (code >= 'A' && code <= 'Z')
        return static_cast<char>(code - 'A' + 'a');
    if (code >= 'a' && code <= 'z')
        return static_cast<char>(code);
    if (code == ';' || code == ':')
        return ';';
    if (code == '\'' || code == '"')
        return '\'';

    const auto ch = key.getTextCharacter();
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<char>(ch - 'A' + 'a');
    if (ch >= 'a' && ch <= 'z')
        return static_cast<char>(ch);
    if (ch == ';' || ch == ':')
        return ';';
    if (ch == '\'' || ch == '"')
        return '\'';
    return 0;
}

bool PianoKeyboard::isPhysicalKeyDown(char key)
{
    const int lower = static_cast<int>(static_cast<unsigned char>(key));
    if (juce::KeyPress::isKeyCurrentlyDown(lower))
        return true;
    if (key >= 'a' && key <= 'z'
        && juce::KeyPress::isKeyCurrentlyDown(key - 'a' + 'A'))
        return true;
    if (key == ';' && juce::KeyPress::isKeyCurrentlyDown(':'))
        return true;
    if (key == '\'' && juce::KeyPress::isKeyCurrentlyDown('"'))
        return true;
    return false;
}

bool PianoKeyboard::handleComputerKeyPress(const juce::KeyPress& key)
{
    if (! computerKeyboardOn_)
        return false;

    const char c = charFromKeyPress(key);
    if (c == 0)
        return false;

    if (isOctaveDownKey(c))
    {
        if (octaveHeld_.insert(c).second)
            shiftOctave(-1);
        return true;
    }
    if (isOctaveUpKey(c))
    {
        if (octaveHeld_.insert(c).second)
            shiftOctave(1);
        return true;
    }

    const int midi = laptopKeyToMidi(c, octave_);
    if (midi < 0)
        return false;

    if (computerNotes_.find(c) == computerNotes_.end())
    {
        computerNotes_[c] = midi;
        beginInteractiveNote(midi);
    }
    return true;
}

bool PianoKeyboard::syncComputerKeyState()
{
    if (! computerKeyboardOn_)
        return false;

    bool consumed = false;

    for (auto it = octaveHeld_.begin(); it != octaveHeld_.end();)
    {
        if (! isPhysicalKeyDown(*it))
        {
            it = octaveHeld_.erase(it);
            consumed = true;
        }
        else
        {
            ++it;
        }
    }

    std::vector<char> released;
    for (const auto& [key, midi] : computerNotes_)
    {
        juce::ignoreUnused(midi);
        if (! isPhysicalKeyDown(key))
            released.push_back(key);
    }
    for (char key : released)
    {
        const auto it = computerNotes_.find(key);
        if (it == computerNotes_.end())
            continue;
        endInteractiveNote(it->second);
        computerNotes_.erase(it);
        consumed = true;
    }

    return consumed;
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

        if (computerKeyboardOn_)
        {
            if (const auto label = laptopKeyForMidi(k.midi, octave_))
            {
                g.setColour(isHighlighted(k.midi) ? juce::Colour(0xff102030) : muted.darker(0.15f));
                g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
                auto labelArea = k.bounds.reduced(0.6f, 0.0f);
                if (k.midi % 12 == 0)
                    labelArea.removeFromBottom(16.0f);
                g.drawText(juce::String::charToString(static_cast<juce::juce_wchar>(*label)),
                           labelArea.removeFromBottom(18.0f), juce::Justification::centred);
            }
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

        if (computerKeyboardOn_)
        {
            if (const auto label = laptopKeyForMidi(k.midi, octave_))
            {
                g.setColour(isHighlighted(k.midi) ? juce::Colour(0xff102030) : juce::Colour(0xffd8d4cc));
                g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
                g.drawText(juce::String::charToString(static_cast<juce::juce_wchar>(*label)),
                           r.removeFromBottom(16.0f), juce::Justification::centred);
            }
        }
    }
}

} // namespace chords
