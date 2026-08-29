#include "gui/SectionComponent.h"
#include "gui/AppLookAndFeel.h"

namespace chords
{

MoreButton::MoreButton()
    : juce::Button("more")
{
    setTooltip("Section options");
}

void MoreButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    auto fill = look != nullptr ? look->accent() : juce::Colour(0xff3d7de0);
    if (down)
        fill = fill.brighter(0.16f);
    else if (highlighted)
        fill = fill.brighter(0.08f);

    auto r = getLocalBounds().toFloat().reduced(2.0f);
    const float side = juce::jmin(r.getWidth(), r.getHeight());
    juce::Rectangle<float> circle(0.0f, 0.0f, side, side);
    circle.setCentre(r.getCentre());

    g.setColour(fill);
    g.fillEllipse(circle);

    g.setColour(juce::Colours::white);
    const float cx = circle.getCentreX();
    const float cy = circle.getCentreY();
    for (int i = -1; i <= 1; ++i)
        g.fillEllipse(cx + static_cast<float>(i) * 4.4f - 1.5f, cy - 1.5f, 3.0f, 3.0f);
}

RepeatSignButton::RepeatSignButton()
    : juce::Button("repeat")
{
    setClickingTogglesState(true);
    setTooltip("Repeat this row once");
}

void RepeatSignButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto muted = look != nullptr ? look->muted() : juce::Colour(0xff8a8694);
    const auto play = look != nullptr ? look->play() : juce::Colour(0xff6fbf9a);
    const auto text = look != nullptr ? look->text() : juce::Colour(0xffe8e4da);
    const bool on = getToggleState();

    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    if (highlighted || down || on)
    {
        auto bg = on ? play.withAlpha(0.22f) : muted.withAlpha(0.12f);
        if (down)
            bg = bg.brighter(0.12f);
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 5.0f);
    }

    auto colour = on ? play : muted;
    if (highlighted && ! on)
        colour = text.withAlpha(0.75f);
    else if (highlighted)
        colour = play.brighter(0.12f);

    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float h = bounds.getHeight() * 0.62f;
    const float top = cy - h * 0.5f;

    const float dotR = 2.15f;
    const float dotsX = cx - 6.2f;
    g.setColour(colour);
    g.fillEllipse(dotsX - dotR, cy - 7.4f - dotR, dotR * 2.0f, dotR * 2.0f);
    g.fillEllipse(dotsX - dotR, cy + 7.4f - dotR, dotR * 2.0f, dotR * 2.0f);

    const float thinX = cx + 0.4f;
    g.fillRect(thinX, top, 1.6f, h);
    g.fillRoundedRectangle(thinX + 3.6f, top, 3.6f, h, 0.8f);
}

SectionComponent::SectionComponent(Song& song, int sectionIndex)
    : song_(song), sectionIndex_(sectionIndex)
{
    more_.onClick = [this] { showMoreMenu(); };
    addAndMakeVisible(more_);

    name_.setEditable(false, true, false);
    name_.setJustificationType(juce::Justification::centredLeft);
    name_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    name_.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    name_.onTextChange = [this] {
        song_.setSectionName(sectionIndex_, name_.getText().toStdString());
    };
    addAndMakeVisible(name_);

    edit_.setComponentID("link");
    edit_.setTooltip("Edit section");
    if (auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel()))
        edit_.setColour(juce::TextButton::textColourOffId, look->accent());
    edit_.onClick = [this] { showEditMenu(); };
    addAndMakeVisible(edit_);

    rebuild();
}

void SectionComponent::showMoreMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rename");
    menu.addItem(2, "Delete section", song_.sections().size() > 1);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more_),
        [this](int result) {
            if (result == 1)
                startRename();
            else if (result == 2)
                song_.removeSection(sectionIndex_);
        });
}

void SectionComponent::showEditMenu()
{
    if (sectionIndex_ < 0 || sectionIndex_ >= static_cast<int>(song_.sections().size()))
        return;

    const auto& section = song_.sections()[static_cast<size_t>(sectionIndex_)];
    const auto label = section.timeSig.label();

    juce::PopupMenu menu;
    menu.addSectionHeader("Time signature");
    menu.addItem(1, "4/4  (4 bars)", true, label == "4/4");
    menu.addItem(2, "3/4  (3 bars)", true, label == "3/4");
    menu.addItem(3, "2/4  (2 bars)", true, label == "2/4");
    menu.addItem(4, "6/8  (6 bars)", true, label == "6/8");
    menu.addSeparator();
    menu.addItem(10, "Rename");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&edit_),
        [this](int result) {
            if (result == 10)
                startRename();
            else if (result > 0)
                applyTimeSignature(result);
        });
}

void SectionComponent::applyTimeSignature(int menuId)
{
    TimeSignature ts { 4, 4 };
    switch (menuId)
    {
        case 2: ts = { 3, 4 }; break;
        case 3: ts = { 2, 4 }; break;
        case 4: ts = { 6, 8 }; break;
        default: break;
    }
    song_.setTimeSignature(sectionIndex_, ts);
}

void SectionComponent::startRename()
{
    name_.showEditor();
}

void SectionComponent::rebuild()
{
    measures_.clear();
    repeats_.clear();
    if (sectionIndex_ < 0 || sectionIndex_ >= static_cast<int>(song_.sections().size()))
        return;

    const auto& section = song_.sections()[static_cast<size_t>(sectionIndex_)];
    name_.setText(section.name, juce::dontSendNotification);

    for (int i = 0; i < static_cast<int>(section.measures.size()); ++i)
    {
        auto m = std::make_unique<MeasureComponent>(song_, sectionIndex_, i);
        m->onSlotSelected = [this, i](int slot) {
            if (onSlotSelected)
                onSlotSelected(i, slot);
        };
        addAndMakeVisible(*m);
        measures_.push_back(std::move(m));
    }

    const int rows = rowCount();
    for (int row = 0; row < rows; ++row)
    {
        auto sign = std::make_unique<RepeatSignButton>();
        sign->setToggleState(song_.rowRepeats(sectionIndex_, row), juce::dontSendNotification);
        sign->onClick = [this, row] {
            song_.setRowRepeat(sectionIndex_, row, repeats_[static_cast<size_t>(row)]->getToggleState());
        };
        addAndMakeVisible(*sign);
        repeats_.push_back(std::move(sign));
    }

    setPlayhead(playingMeasure_, playingSlot_);
    resized();
}

void SectionComponent::setPlayhead(int measure, int slot)
{
    playingMeasure_ = measure;
    playingSlot_ = slot;
    for (int i = 0; i < static_cast<int>(measures_.size()); ++i)
        measures_[static_cast<size_t>(i)]->setPlayingSlot(i == measure ? slot : -1);
}

int SectionComponent::rowCount() const
{
    return Song::rowCount(static_cast<int>(measures_.size()));
}

int SectionComponent::measureWidth(int areaWidth) const
{
    const int barArea = juce::jmax(80, areaWidth - kRepeatW - kGap);
    const int gaps = kGap * (Song::kBarsPerRow - 1);
    return juce::jmax(56, (barArea - gaps) / Song::kBarsPerRow);
}

int SectionComponent::preferredHeight(int width) const
{
    juce::ignoreUnused(width);
    const int rows = juce::jmax(1, rowCount());
    return kHeaderH + 4 + rows * (kChordH + 4) + 4;
}

void SectionComponent::layoutRow(juce::Rectangle<int> area)
{
    const int n = static_cast<int>(measures_.size());
    if (n <= 0)
        return;

    const int boxW = measureWidth(area.getWidth());
    const int rows = rowCount();

    for (int row = 0; row < rows; ++row)
    {
        const int start = row * Song::kBarsPerRow;
        const int end = juce::jmin(start + Song::kBarsPerRow, n);
        const int y = area.getY() + row * (kChordH + 4);
        int x = area.getX();

        for (int i = start; i < end; ++i)
        {
            measures_[static_cast<size_t>(i)]->setBounds(x, y, boxW, kChordH);
            x += boxW + kGap;
        }

        if (row < static_cast<int>(repeats_.size()))
        {
            const int signX = area.getRight() - kRepeatW;
            repeats_[static_cast<size_t>(row)]->setBounds(signX, y, kRepeatW, kChordH);
        }
    }
}

void SectionComponent::lookAndFeelChanged()
{
    if (auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel()))
        edit_.setColour(juce::TextButton::textColourOffId, look->accent());
}

void SectionComponent::paint(juce::Graphics&) {}

void SectionComponent::resized()
{
    auto r = getLocalBounds().reduced(4, 2);
    auto header = r.removeFromTop(kHeaderH);
    more_.setBounds(header.removeFromLeft(32).withSizeKeepingCentre(28, 28));
    header.removeFromLeft(6);
    edit_.setBounds(header.removeFromRight(52).reduced(0, 4));
    name_.setBounds(header);

    r.removeFromTop(2);
    layoutRow(r);
}

} // namespace chords
