#include "gui/SectionComponent.h"

namespace chords
{

void TimeSigView::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto text = look != nullptr ? look->text() : juce::Colours::whitesmoke;
    auto r = getLocalBounds().toFloat();
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    auto top = r.removeFromTop(r.getHeight() * 0.5f);
    g.drawText(juce::String(ts_.numerator), top, juce::Justification::centred);
    g.setColour(text.withAlpha(0.35f));
    g.drawLine(r.getX() + 4.0f, r.getY(), r.getRight() - 4.0f, r.getY(), 1.0f);
    g.setColour(text);
    g.drawText(juce::String(ts_.denominator), r, juce::Justification::centred);
}

SectionComponent::SectionComponent(Song& song, int sectionIndex)
    : song_(song), sectionIndex_(sectionIndex)
{
    name_.setEditable(false, true, false);
    name_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    name_.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    name_.onTextChange = [this] {
        song_.setSectionName(sectionIndex_, name_.getText().toStdString());
    };
    addAndMakeVisible(name_);

    timeSig_.addItem("4/4", 1);
    timeSig_.addItem("3/4", 2);
    timeSig_.addItem("2/4", 3);
    timeSig_.addItem("6/8", 4);
    timeSig_.onChange = [this] {
        TimeSignature ts { 4, 4 };
        switch (timeSig_.getSelectedId())
        {
            case 2: ts = { 3, 4 }; break;
            case 3: ts = { 2, 4 }; break;
            case 4: ts = { 6, 8 }; break;
            default: break;
        }
        song_.setTimeSignature(sectionIndex_, ts);
    };
    addAndMakeVisible(timeSig_);
    addAndMakeVisible(timeSigView_);

    addMeasure_.setTooltip("Add a bar");
    addMeasure_.onClick = [this] { song_.addMeasure(sectionIndex_); };
    addAndMakeVisible(addMeasure_);

    remove_.setTooltip("Delete section");
    remove_.onClick = [this] { song_.removeSection(sectionIndex_); };
    addAndMakeVisible(remove_);

    rebuild();
}

void SectionComponent::rebuild()
{
    measures_.clear();
    if (sectionIndex_ < 0 || sectionIndex_ >= static_cast<int>(song_.sections().size()))
        return;

    const auto& section = song_.sections()[static_cast<size_t>(sectionIndex_)];
    name_.setText(section.name, juce::dontSendNotification);
    timeSigView_.set(section.timeSig);

    const auto label = section.timeSig.label();
    if (label == "3/4") timeSig_.setSelectedId(2, juce::dontSendNotification);
    else if (label == "2/4") timeSig_.setSelectedId(3, juce::dontSendNotification);
    else if (label == "6/8") timeSig_.setSelectedId(4, juce::dontSendNotification);
    else timeSig_.setSelectedId(1, juce::dontSendNotification);

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

int SectionComponent::preferredHeight(int width) const
{
    const int usable = juce::jmax(120, width - 50);
    int x = 0;
    int rows = 1;
    if (measures_.empty())
        rows = 1;
    for (const auto& m : measures_)
    {
        const int w = m->preferredWidth();
        if (x > 0 && x + w > usable)
        {
            ++rows;
            x = 0;
        }
        x += w + 4;
    }
    return kHeaderH + 8 + rows * kRowH + 8;
}

void SectionComponent::layoutMeasures(juce::Rectangle<int> area, bool apply) const
{
    int x = area.getX();
    int y = area.getY();
    const int right = area.getRight();
    for (auto& m : measures_)
    {
        const int w = m->preferredWidth();
        if (x > area.getX() && x + w > right)
        {
            x = area.getX();
            y += kRowH;
        }
        if (apply)
            m->setBounds(x, y, w, kRowH);
        x += w + 4;
    }
}

void SectionComponent::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    const auto panel = look != nullptr ? look->panel() : juce::Colour(0xff1c1f2a);
    const auto text = look != nullptr ? look->text() : juce::Colours::whitesmoke;
    auto r = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(panel);
    g.fillRoundedRectangle(r, 10.0f);
    juce::ignoreUnused(text);
}

void SectionComponent::resized()
{
    auto r = getLocalBounds().reduced(8, 6);
    auto header = r.removeFromTop(kHeaderH - 4);
    remove_.setBounds(header.removeFromRight(28).reduced(2));
    addMeasure_.setBounds(header.removeFromRight(64).reduced(2));
    timeSig_.setBounds(header.removeFromRight(64).reduced(2, 4));
    name_.setBounds(header.withTrimmedRight(8));

    r.removeFromTop(2);
    timeSigView_.setBounds(r.removeFromLeft(36).withHeight(48).withY(r.getY() + 8));
    layoutMeasures(r, true);
}

} // namespace chords
