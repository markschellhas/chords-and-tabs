#include "MainComponent.h"

class ChordsAndTabsApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Chords & Tabs"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow_ = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow_ = nullptr; }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name, juce::Colour(0xff12141a), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new chords::MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(860, 640, 10000, 10000);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow_;
};

START_JUCE_APPLICATION(ChordsAndTabsApplication)
