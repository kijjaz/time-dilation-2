#include <juce_gui_extra/juce_gui_extra.h>
#include "gui/WorkstationContainerComponent.h"
#include "utils/AgentTestRunner.h"
#include "utils/TerminalCommandProcessor.h"
#include <iostream>

class TimeDilationApplication  : public juce::JUCEApplication
{
public:
    TimeDilationApplication() {}

    const juce::String getApplicationName() override      { return "Time Dilation DAW"; }
    const juce::String getApplicationVersion() override   { return "0.0.3"; }
    bool moreThanOneInstanceAllowed() override            { return true; }

    void initialise(const juce::String& commandLine) override
    {
        auto args = getCommandLineParameterArray();
        
        // Headless Agent Testing Mode
        if (commandLine.contains("--agent-test") || commandLine.contains("--headless-test"))
        {
            int result = TimeDilationDAW::AgentTestRunner::runHeadlessTest(args);
            setApplicationReturnValue(result);
            quit();
            return;
        }

        // Headless Interactive CLI Terminal Mode
        if (commandLine.contains("--cli") || commandLine.contains("--interactive"))
        {
            TimeDilationDAW::WorkstationContainerComponent workstation(false);
            workstation.setSize(1280, 720);
            TimeDilationDAW::TerminalCommandProcessor::runInteractiveLoop(workstation);
            setApplicationReturnValue(0);
            quit();
            return;
        }

        // Single Command Execution Mode (e.g. --cmd="add osc~ sin 440")
        if (commandLine.contains("--cmd="))
        {
            TimeDilationDAW::WorkstationContainerComponent workstation(false);
            workstation.setSize(1280, 720);

            for (const auto& arg : args)
            {
                if (arg.startsWith("--cmd="))
                {
                    juce::String cmdText = arg.substring(6);
                    std::string res = TimeDilationDAW::TerminalCommandProcessor::processCommand(workstation, cmdText.toStdString());
                    std::cout << res << std::endl;
                }
            }
            setApplicationReturnValue(0);
            quit();
            return;
        }

        // GUI Window Mode
        mainWindow = std::make_unique<MainWindow>(getApplicationName());

        if (commandLine.contains("--auto-close") || commandLine.contains("--test-and-exit"))
        {
            juce::Timer::callAfterDelay(1500, [this]() {
                setApplicationReturnValue(0);
                quit();
            });
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                         .findColour(juce::ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new TimeDilationDAW::WorkstationContainerComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            setResizeLimits(800, 600, 3840, 2160);
            centreWithSize(1280, 720);
           #endif

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
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(TimeDilationApplication)
