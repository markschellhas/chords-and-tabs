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

juce::File MainComponent::prefsFile()
{
    return settingsDirectory().getChildFile("prefs.xml");
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
    loadInstrumentPref();
    if (deviceManager_.getCurrentAudioDevice() == nullptr)
        deviceManager_.initialise(0, 2, nullptr, true);
    deviceManager_.addAudioCallback(&engine_);
    engine_.setSong(song_);
    engine_.setBpm(song_.bpm());

    title_.setJustificationType(juce::Justification::centred);
    title_.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    hint_.setJustificationType(juce::Justification::centred);
    hint_.setColour(juce::Label::textColourId, lookAndFeel_.muted());
    hint_.setFont(juce::Font(juce::FontOptions(13.0f)));
    updateHint();

    transport_.onDevice = [this] { showDeviceDialog(); };

    circle_.onSelectionChanged = [this](int) {
        previewChord_.reset();
        setNavRegion(NavRegion::Circle);
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
        setNavRegion(NavRegion::Song);
        updateKeyboardHighlight();
        grabKeyboardFocus();
    };

    song_.addListener([this] {
        engine_.setSong(song_);
        publishAgentState();
    });

    agentServer_.start(); // loopback API; snapshots remain if the port is taken
    publishAgentState();

    piano_.setSoundName(engine_.instrumentName());
    piano_.onCycleSound = [this](int delta) {
        setNavRegion(NavRegion::Keyboard);
        cycleSound(delta);
        grabKeyboardFocus();
    };

    addAndMakeVisible(title_);
    addAndMakeVisible(hint_);
    addAndMakeVisible(transport_);
    addAndMakeVisible(circle_);
    addAndMakeVisible(sections_);
    addAndMakeVisible(piano_);

    circle_.addMouseListener(this, true);
    sections_.addMouseListener(this, true);
    piano_.addMouseListener(this, true);

    startTimerHz(30);
    setSize(1080, 820);
}

MainComponent::~MainComponent()
{
    circle_.removeMouseListener(this);
    sections_.removeMouseListener(this);
    piano_.removeMouseListener(this);
    agentServer_.stop();
    saveDeviceState();
    saveInstrumentPref();
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

void MainComponent::loadInstrumentPref()
{
    const auto file = prefsFile();
    if (! file.existsAsFile())
        return;
    if (auto xml = juce::XmlDocument::parse(file))
        engine_.setInstrument(instrumentFromIndex(xml->getIntAttribute("instrument", 0)));
}

void MainComponent::saveInstrumentPref()
{
    juce::XmlElement xml("prefs");
    xml.setAttribute("instrument", static_cast<int>(engine_.instrument()));
    const auto file = prefsFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo(file);
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

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    if (auto* c = navComponent(navRegion_))
        lookAndFeel_.drawNavFocusFrame(g, c->getBounds().toFloat());
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    auto* src = e.eventComponent;
    if (src == nullptr)
        return;

    if (src == &circle_ || circle_.isParentOf(src))
        setNavRegion(NavRegion::Circle);
    else if (src == &sections_ || sections_.isParentOf(src))
        setNavRegion(NavRegion::Song);
    else if (src == &piano_ || piano_.isParentOf(src))
        setNavRegion(NavRegion::Keyboard);
}

void MainComponent::setNavRegion(NavRegion region)
{
    const bool changed = navRegion_ != region;
    navRegion_ = region;
    if (changed)
        updateHint();
    grabKeyboardFocus();
    if (changed)
        repaint();
}

void MainComponent::updateHint()
{
    juce::String text;
    switch (navRegion_)
    {
        case NavRegion::Circle:
            text = "j/k move between areas    h/l or left/right change key    space plays";
            break;
        case NavRegion::Song:
            text = "j/k move between areas    space plays";
            break;
        case NavRegion::Keyboard:
            text = "j/k move between areas    h/l change sound    space plays";
            break;
    }
    hint_.setText(text, juce::dontSendNotification);
}

void MainComponent::cycleSound(int delta)
{
    engine_.cycleInstrument(delta);
    piano_.setSoundName(engine_.instrumentName());
    saveInstrumentPref();
}

juce::Component* MainComponent::navComponent(NavRegion region)
{
    switch (region)
    {
        case NavRegion::Circle:   return &circle_;
        case NavRegion::Song:     return &sections_;
        case NavRegion::Keyboard: return &piano_;
    }
    return &circle_;
}

int MainComponent::vimLetter(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown())
        return 0;

    int raw = static_cast<int>(key.getTextCharacter());
    if (raw == 0)
        raw = key.getKeyCode();
    if (raw >= 'A' && raw <= 'Z')
        raw += 'a' - 'A';
    return raw;
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop(52);
    title_.setBounds(top.removeFromTop(30));
    hint_.setBounds(top);

    auto transport = r.removeFromTop(44);
    transport_.setBounds(transport);

    auto keys = r.removeFromBottom(146);
    piano_.setBounds(keys.reduced(8, 6));

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

    const int ch = vimLetter(key);
    if (ch == 'j')
    {
        setNavRegion(cycleNavRegion(navRegion_, 1));
        return true;
    }
    if (ch == 'k')
    {
        setNavRegion(cycleNavRegion(navRegion_, -1));
        return true;
    }
    if (ch == 'h')
    {
        if (navRegion_ == NavRegion::Circle)
        {
            circle_.rotate(-1);
            return true;
        }
        if (navRegion_ == NavRegion::Keyboard)
        {
            cycleSound(-1);
            return true;
        }
        return false;
    }
    if (ch == 'l')
    {
        if (navRegion_ == NavRegion::Circle)
        {
            circle_.rotate(1);
            return true;
        }
        if (navRegion_ == NavRegion::Keyboard)
        {
            cycleSound(1);
            return true;
        }
        return false;
    }
    return false;
}

} // namespace chords
