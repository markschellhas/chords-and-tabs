#include "gui/AppLookAndFeel.h"

namespace chords
{

AppLookAndFeel::AppLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bg_);
    setColour(juce::Label::textColourId, text_);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, panel_);
    setColour(juce::TextButton::textColourOffId, text_);
    setColour(juce::TextButton::textColourOnId, accent_);
    setColour(juce::ToggleButton::textColourId, text_);
    setColour(juce::Slider::thumbColourId, accent_);
    setColour(juce::Slider::trackColourId, juce::Colour(0xff2a3144));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff151820));
    setColour(juce::Slider::textBoxTextColourId, text_);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff151820));
    setColour(juce::Slider::textBoxOutlineColourId, muted_.withAlpha(0.35f));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff151820));
    setColour(juce::ComboBox::outlineColourId, muted_.withAlpha(0.35f));
    setColour(juce::ComboBox::textColourId, text_);
    setColour(juce::ComboBox::arrowColourId, muted_);
    setColour(juce::PopupMenu::backgroundColourId, panel_);
    setColour(juce::PopupMenu::textColourId, text_);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, wedgeActive_.withAlpha(0.45f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff151820));
    setColour(juce::TextEditor::textColourId, text_);
    setColour(juce::TextEditor::outlineColourId, muted_.withAlpha(0.35f));
    setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour(juce::CaretComponent::caretColourId, accent_);
    setColour(juce::ScrollBar::thumbColourId, muted_.withAlpha(0.5f));
}

juce::Font AppLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(juce::jmin(15.0f, buttonHeight * 0.48f)));
}

juce::Font AppLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(14.0f));
}

juce::Font AppLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(14.0f));
}

void AppLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    if (button.getComponentID() == "link")
    {
        if (highlighted || down)
        {
            g.setColour(accent_.withAlpha(down ? 0.22f : 0.12f));
            g.fillRoundedRectangle(bounds, 6.0f);
        }
        return;
    }

    auto bg = backgroundColour;
    if (down)
        bg = bg.brighter(0.16f);
    else if (highlighted)
        bg = bg.brighter(0.08f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(muted_.withAlpha(button.isEnabled() ? 0.4f : 0.2f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour(accent_.withAlpha(0.28f));
        g.fillRoundedRectangle(bounds.reduced(1.5f), 5.0f);
    }
}

void AppLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                      bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool on = button.getToggleState();
    auto bg = on ? play_.withAlpha(0.85f) : panel_;
    if (down)
        bg = bg.brighter(0.1f);
    else if (highlighted)
        bg = bg.brighter(0.06f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(on ? play_.brighter(0.25f) : muted_.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    g.setColour(on ? juce::Colour(0xff102018) : text_);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void AppLookAndFeel::drawNavFocusFrame(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    bounds = bounds.reduced(1.5f);
    g.setColour(accent_.withAlpha(0.22f));
    g.drawRoundedRectangle(bounds.expanded(1.0f), 10.0f, 4.0f);
    g.setColour(accent_);
    g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
}

void AppLookAndFeel::drawChordChip(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   const juce::String& name,
                                   bool filled, bool playing, bool dropHover,
                                   juce::Colour chip, juce::Colour chipText,
                                   juce::Colour accent, juce::Colour muted,
                                   float progress)
{
    const float r = 5.0f;
    if (dropHover)
    {
        g.setColour(accent.withAlpha(0.95f));
        g.fillRoundedRectangle(bounds, r);
        g.setColour(accent.brighter(0.25f));
        g.drawRoundedRectangle(bounds, r, 1.4f);
    }
    else if (filled || playing)
    {
        const float p = playing ? juce::jlimit(0.0f, 1.0f, progress) : 0.0f;
        {
            juce::Graphics::ScopedSaveState state(g);
            juce::Path clip;
            clip.addRoundedRectangle(bounds, r);
            g.reduceClipRegion(clip);
            g.setColour(chip);
            g.fillRect(bounds);
            if (p > 0.0f)
            {
                g.setColour(chip.overlaidWith(juce::Colours::white.withAlpha(0.38f)));
                g.fillRect(bounds.withWidth(bounds.getWidth() * p));
            }
        }
        if (playing)
        {
            g.setColour(accent.withAlpha(0.7f));
            g.drawRoundedRectangle(bounds, r, 1.2f);
        }
    }
    else
    {
        g.setColour(chip.withMultipliedAlpha(0.28f));
        g.fillRoundedRectangle(bounds, r);
    }

    if (filled || playing || dropHover)
    {
        g.setColour(dropHover ? juce::Colour(0xff10141c) : chipText);
        g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
        g.drawText(name, bounds.reduced(8.0f, 0.0f), juce::Justification::centred);
    }
    juce::ignoreUnused(muted);
}

} // namespace chords
