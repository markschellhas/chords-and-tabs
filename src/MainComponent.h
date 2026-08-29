#pragma once

#include "audio/ChordEngine.h"
#include "gui/AppLookAndFeel.h"
#include "gui/CircleOfFifthsComponent.h"
#include "gui/PianoKeyboard.h"
#include "gui/SectionListComponent.h"
#include "gui/TransportStrip.h"
#include "model/Song.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <optional>

namespace chords
{

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      private juce::Timer,
                      private juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyPressed(const juce::KeyPress& key) override { return keyPressed(key, this); }

private:
    void timerCallback() override;
    void showDeviceDialog();
    void updateKeyboardHighlight();
    static juce::File settingsFile();
    void loadDeviceState();
    void saveDeviceState();

    AppLookAndFeel lookAndFeel_;
    juce::AudioDeviceManager deviceManager_;
    Song song_;
    ChordEngine engine_;

    juce::Label title_ { {}, "Chords & Tabs" };
    juce::Label hint_ { {}, "Left / Right change key    drag onto a chord to split    drag edges to open slots    space plays" };
    TransportStrip transport_;
    CircleOfFifthsComponent circle_;
    SectionListComponent sections_;
    PianoKeyboard piano_;

    std::optional<Chord> previewChord_;
    std::optional<Chord> selectedChord_;
};

} // namespace chords
