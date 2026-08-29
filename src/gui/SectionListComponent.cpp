#include "gui/SectionListComponent.h"
#include "gui/AppLookAndFeel.h"

namespace chords
{

SectionListComponent::SectionListComponent(Song& song)
    : song_(song)
{
    viewport_.setViewedComponent(&content_, false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);

    addSection_.setComponentID("link");
    addSection_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7eb6e8));
    addSection_.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem(1, "Verse");
        menu.addItem(2, "Chorus");
        menu.addItem(3, "Pre-Chorus");
        menu.addItem(4, "Bridge");
        menu.addItem(5, "Intro");
        menu.addItem(6, "Outro");
        menu.addItem(7, "Solo");
        menu.addItem(8, "Custom...");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addSection_),
            [this](int result) {
                if (result == 0)
                    return;
                if (result == 8)
                {
                    auto* editor = new juce::AlertWindow("New section", "Name this section",
                                                         juce::MessageBoxIconType::NoIcon);
                    editor->addTextEditor("name", "Section", "Name");
                    editor->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
                    editor->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                    editor->enterModalState(true, juce::ModalCallbackFunction::create(
                        [this, editor](int r) {
                            if (r == 1)
                            {
                                auto name = editor->getTextEditorContents("name").toStdString();
                                song_.addSection(name.empty() ? "Section" : name);
                            }
                        }), true);
                    return;
                }

                static const char* names[] = { "", "Verse", "Chorus", "Pre-Chorus", "Bridge",
                                               "Intro", "Outro", "Solo" };
                song_.addSection(names[result]);
            });
    };
    content_.addAndMakeVisible(addSection_);

    song_.addListener([this] {
        const auto safe = juce::Component::SafePointer<SectionListComponent>(this);
        juce::MessageManager::callAsync([safe] {
            if (safe)
                safe->rebuild();
        });
    });

    rebuild();
}

void SectionListComponent::rebuild()
{
    sections_.clear();
    for (int i = 0; i < static_cast<int>(song_.sections().size()); ++i)
    {
        auto s = std::make_unique<SectionComponent>(song_, i);
        s->onSlotSelected = [this, i](int measure, int slot) {
            if (onSlotSelected)
                onSlotSelected(i, measure, slot);
        };
        s->onSlotAudition = [this, i](int measure, int slot) {
            if (onSlotAudition)
                onSlotAudition(i, measure, slot);
        };
        content_.addAndMakeVisible(*s);
        sections_.push_back(std::move(s));
    }
    content_.addAndMakeVisible(addSection_);
    setPlayhead(playSection_, playMeasure_, playSlot_, playing_, playProgress_);
    resized();
}

void SectionListComponent::setPlayhead(int section, int measure, int slot, bool playing, float progress)
{
    playSection_ = section;
    playMeasure_ = measure;
    playSlot_ = slot;
    playing_ = playing;
    playProgress_ = playing ? progress : 0.0f;
    for (int i = 0; i < static_cast<int>(sections_.size()); ++i)
        sections_[static_cast<size_t>(i)]->setPlayhead(
            playing && i == section ? measure : -1,
            playing && i == section ? slot : -1,
            playing && i == section ? playProgress_ : 0.0f);
}

void SectionListComponent::lookAndFeelChanged()
{
    if (auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel()))
        addSection_.setColour(juce::TextButton::textColourOffId, look->accent());
}

void SectionListComponent::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<AppLookAndFeel*>(&getLookAndFeel());
    g.fillAll(look != nullptr ? look->background() : juce::Colour(0xff12141a));
}

void SectionListComponent::resized()
{
    viewport_.setBounds(getLocalBounds());
    const int width = juce::jmax(200, viewport_.getMaximumVisibleWidth());
    int y = 4;
    for (auto& s : sections_)
    {
        const int h = s->preferredHeight(width - 8);
        s->setBounds(4, y, width - 8, h);
        y += h + 6;
    }
    addSection_.setBounds(4, y, 168, 28);
    y += 36;
    content_.setSize(width, juce::jmax(y, viewport_.getHeight()));
}

} // namespace chords
