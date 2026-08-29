#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace chords
{

class AppLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppLookAndFeel();

    juce::Colour background() const { return bg_; }
    juce::Colour panel() const { return panel_; }
    juce::Colour circleBackground() const { return circleBg_; }
    juce::Colour wedge() const { return wedge_; }
    juce::Colour wedgeActive() const { return wedgeActive_; }
    juce::Colour wedgeInnerActive() const { return wedgeInnerActive_; }
    juce::Colour text() const { return text_; }
    juce::Colour muted() const { return muted_; }
    juce::Colour chip() const { return chip_; }
    juce::Colour chipText() const { return chipText_; }
    juce::Colour accent() const { return accent_; }
    juce::Colour play() const { return play_; }
    juce::Colour danger() const { return danger_; }
    juce::Colour keyboardWhite() const { return keyWhite_; }
    juce::Colour keyboardBlack() const { return keyBlack_; }
    juce::Colour keyHighlight() const { return keyHighlight_; }

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    void drawNavFocusFrame(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    static void drawChordChip(juce::Graphics& g, juce::Rectangle<float> bounds,
                              const juce::String& name,
                              bool filled, bool playing, bool dropHover,
                              juce::Colour chip, juce::Colour chipText,
                              juce::Colour accent, juce::Colour muted,
                              float progress = 0.0f);

private:
    juce::Colour bg_ { 0xff12141a };
    juce::Colour panel_ { 0xff1c1f2a };
    juce::Colour circleBg_ { 0xff0d1018 };
    juce::Colour wedge_ { 0xff232838 };
    juce::Colour wedgeActive_ { 0xff6a7bb5 };
    juce::Colour wedgeInnerActive_ { 0xff8b97d0 };
    juce::Colour text_ { 0xffe8e4da };
    juce::Colour muted_ { 0xff8a8694 };
    juce::Colour chip_ { 0xff3c6fd4 };
    juce::Colour chipText_ { 0xffeef2ff };
    juce::Colour accent_ { 0xff7eb6e8 };
    juce::Colour play_ { 0xff6fbf9a };
    juce::Colour danger_ { 0xffc45d55 };
    juce::Colour keyWhite_ { 0xffe7e4dc };
    juce::Colour keyBlack_ { 0xff16181f };
    juce::Colour keyHighlight_ { 0xff7eb6e8 };
};

} // namespace chords
