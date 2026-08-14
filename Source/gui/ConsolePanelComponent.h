#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../utils/ConsoleLogger.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

class ConsolePanelComponent : public juce::Component,
                              public juce::ListBoxModel
{
public:
    ConsolePanelComponent();
    ~ConsolePanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;

    void refreshLogs();
    void clearLogs();
    void copyAllLogsToClipboard();

    std::function<void()> onCloseRequested;

private:
    juce::Label titleLabel{ "title", "TERMINAL CONSOLE & DEBUG STREAM" };
    juce::TextButton clearButton{ "Clear" };
    juce::ToggleButton autoScrollToggle{ "Auto-Scroll" };
    juce::TextButton copyButton{ "Copy All" };
    juce::TextButton closeButton{ "X" };
    juce::TextEditor searchFilter;

    juce::ListBox logListBox;
    std::vector<LogEntry> displayedEntries;
    std::string currentFilter = "";
    bool autoScrollEnabled = true;

    void updateFilteredList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsolePanelComponent)
};

} // namespace TimeDilationDAW
