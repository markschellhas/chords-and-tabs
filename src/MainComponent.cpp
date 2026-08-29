#include "MainComponent.h"
#include "api/AgentClient.h"
#include "api/SongJson.h"

namespace chords
{

juce::File MainComponent::settingsDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("chords-and-tabs");
}

juce::File MainComponent::settingsFile()
{
    return settingsDirectory().getChildFile("device.xml");
}

void MainComponent::publishAgentState()
{
    const AgentView view { circle_.selectedIndex() };
    const auto songJson = songToJson(song_, view);
    const auto progressionsJson = progressionsToJson(song_, view);

    agentServer_.setSongJson(songJson);
    agentServer_.setProgressionsJson(progressionsJson);

    writeAgentSnapshot("song.json", songJson);
    writeAgentSnapshot("progressions.json", progressionsJson);

    const auto port = agentServer_.port();
    const auto meta = std::string("{\"app\":\"chords-and-tabs\",\"port\":")
                    + std::to_string(port)
                    + ",\"url\":\"http://127.0.0.1:" + std::to_string(port) + "\"}";
    writeAgentSnapshot("agent-api.json", meta);
}

MainComponent::MainComponent()
    : transport_(song_, engine_),
      sections_(song_)
{
    setLookAndFeel(&lookAndFeel_);
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    setOpaque(true);

    loadDeviceState();
    if (deviceManager_.getCurrentAudioDevice() == nullptr)
        deviceManager_.initialise(0, 2, nullptr, true);
    deviceManager_.addAudioCallback(&engine_);
    engine_.setSong(song_);
    engine_.setBpm(song_.bpm());

    title_.setJustificationType(juce::Justification::centred);
    title_.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    hint_.setText("Left / Right change key    drag chords onto bars    space plays",
                  juce::dontSendNotification);
    hint_.setJustificationType(juce::Justification::centred);
    hint_.setColour(juce::Label::textColourId, lookAndFeel_.muted());
    hint_.setFont(juce::Font(juce::FontOptions(13.0f)));

    transport_.onDevice = [this] { showDeviceDialog(); };

    circle_.onSelectionChanged = [this](int) {
        previewChord_.reset();
        updateKeyboardHighlight();
        grabKeyboardFocus();
        publishAgentState();
    };
    circle_.onChordPreview = [this](const Chord& c) {
        previewChord_ = c;
        updateKeyboardHighlight();
    };
    circle_.onPreviewEnd = [this] {
        previewChord_.reset();
        updateKeyboardHighlight();
    };

    sections_.onSlotSelected = [this](int section, int measure, int slot) {
        selectedChord_ = song_.getChord(section, measure, slot);
        updateKeyboardHighlight();
        grabKeyboardFocus();
    };

    song_.addListener([this] {
        engine_.setSong(song_);
        publishAgentState();
    });

    agentServer_.start(); // loopback API; snapshots remain if the port is taken
    publishAgentState();

    addAndMakeVisible(title_);
    addAndMakeVisible(hint_);
    addAndMakeVisible(transport_);
    addAndMakeVisible(circle_);
    addAndMakeVisible(sections_);
    addAndMakeVisible(piano_);

    startTimerHz(30);
    setSize(1080, 820);
}

MainComponent::~MainComponent()
{
    agentServer_.stop();
    saveDeviceState();
    deviceManager_.removeAudioCallback(&engine_);
    setLookAndFeel(nullptr);
}

void MainComponent::loadDeviceState()
{
    const auto file = settingsFile();
    if (! file.existsAsFile())
        return;
    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr)
        deviceManager_.initialise(0, 2, xml.get(), true);
}

void MainComponent::saveDeviceState()
{
    if (auto xml = deviceManager_.createStateXml())
    {
        const auto file = settingsFile();
        file.getParentDirectory().createDirectory();
        xml->writeTo(file);
    }
}

void MainComponent::showDeviceDialog()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager_, 0, 0, 1, 2,
                                                            false, false, true, false);
    selector->setSize(500, 380);
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(selector);
    opts.dialogTitle = "Audio Device";
    opts.dialogBackgroundColour = lookAndFeel_.background();
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = true;
    opts.componentToCentreAround = this;
    opts.launchAsync();
}

void MainComponent::updateKeyboardHighlight()
{
    if (engine_.isPlaying() && engine_.hasSoundingNotes())
    {
        piano_.setHighlightedNotes(engine_.soundingNotes());
        return;
    }

    if (previewChord_)
    {
        piano_.setHighlightedNotes(previewChord_->midiNotes(4));
        return;
    }

    if (selectedChord_)
    {
        piano_.setHighlightedNotes(selectedChord_->midiNotes(4));
        return;
    }

    piano_.setHighlightedNotes(CircleOfFifths::majorChord(circle_.selectedIndex()).midiNotes(4));
}

void MainComponent::timerCallback()
{
    transport_.refresh();

    const bool playing = engine_.isPlaying();
    if (playing)
    {
        if (auto ev = engine_.currentEvent())
            sections_.setPlayhead(ev->sectionIndex, ev->measureIndex, ev->slotIndex, ! ev->rest);
        else
            sections_.setPlayhead(-1, -1, -1, false);
    }
    else
    {
        sections_.setPlayhead(-1, -1, -1, false);
    }

    updateKeyboardHighlight();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(lookAndFeel_.background());
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop(52);
    title_.setBounds(top.removeFromTop(30));
    hint_.setBounds(top);

    auto transport = r.removeFromTop(44);
    transport_.setBounds(transport);

    auto keys = r.removeFromBottom(118);
    piano_.setBounds(keys.reduced(8, 8));

    const int circleH = juce::jlimit(230, 340, r.getHeight() / 3);
    circle_.setBounds(r.removeFromTop(circleH).reduced(8, 4));
    sections_.setBounds(r.reduced(4, 2));
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()) != nullptr)
        return false;

    if (key == juce::KeyPress::spaceKey)
    {
        engine_.togglePlay();
        return true;
    }
    if (key == juce::KeyPress::leftKey)
    {
        circle_.rotate(-1);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        circle_.rotate(1);
        return true;
    }
    return false;
}

} // namespace chords
