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

int SectionComponent::measureWidth(int areaWidth) const
{
    const int n = juce::jmax(1, static_cast<int>(measures_.size()));
    const int gaps = kGap * (n - 1);
    return juce::jmax(56, (areaWidth - gaps) / n);
}

int SectionComponent::preferredHeight(int width) const
{
    const int usable = juce::jmax(120, width - 16);
    const int n = static_cast<int>(measures_.size());
    if (n <= 0)
        return kHeaderH + kChordH + 12;

    const int boxW = measureWidth(usable);
    int x = 0;
    int rows = 1;
    for (int i = 0; i < n; ++i)
    {
        if (x > 0 && x + boxW > usable)
        {
            ++rows;
            x = 0;
        }
        x += boxW + kGap;
    }
    return kHeaderH + 4 + rows * (kChordH + 4) + 4;
}

void SectionComponent::layoutMeasures(juce::Rectangle<int> area, bool apply) const
{
    const int n = static_cast<int>(measures_.size());
    if (n <= 0)
        return;

    const int boxW = measureWidth(area.getWidth());
    int x = area.getX();
    int y = area.getY();
    const int right = area.getRight();

    for (auto& m : measures_)
    {
        if (x > area.getX() && x + boxW > right)
        {
            x = area.getX();
            y += kChordH + 4;
        }
        if (apply)
            m->setBounds(x, y, boxW, kChordH);
        x += boxW + kGap;
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
    layoutMeasures(r, true);
}

} // namespace chords
