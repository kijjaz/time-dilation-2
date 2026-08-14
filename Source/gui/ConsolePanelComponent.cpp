#include "ConsolePanelComponent.h"

namespace TimeDilationDAW
{

ConsolePanelComponent::ConsolePanelComponent()
{
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(titleLabel);

    clearButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.2f));
    clearButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::goldAccent);
    clearButton.onClick = [this]() { clearLogs(); };
    addAndMakeVisible(clearButton);

    autoScrollToggle.setToggleState(true, juce::dontSendNotification);
    autoScrollToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    autoScrollToggle.setColour(juce::ToggleButton::tickColourId, CarbonGoldLookAndFeel::cyberCyan);
    autoScrollToggle.onClick = [this]() { autoScrollEnabled = autoScrollToggle.getToggleState(); };
    addAndMakeVisible(autoScrollToggle);

    copyButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.2f));
    copyButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::cyberCyan);
    copyButton.onClick = [this]() { copyAllLogsToClipboard(); };
    addAndMakeVisible(copyButton);

    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    closeButton.onClick = [this]() { if (onCloseRequested) onCloseRequested(); };
    addAndMakeVisible(closeButton);

    searchFilter.setTextToShowWhenEmpty("Filter tags or text...", juce::Colours::grey);
    searchFilter.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::carbonBg);
    searchFilter.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::cyberCyan);
    searchFilter.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
    searchFilter.onTextChange = [this]() {
        currentFilter = searchFilter.getText().toLowerCase().toStdString();
        updateFilteredList();
    };
    addAndMakeVisible(searchFilter);

    logListBox.setModel(this);
    logListBox.setRowHeight(19);
    logListBox.setColour(juce::ListBox::backgroundColourId, CarbonGoldLookAndFeel::carbonBg);
    logListBox.setColour(juce::ScrollBar::thumbColourId, CarbonGoldLookAndFeel::goldAccent.withAlpha(0.6f));
    addAndMakeVisible(logListBox);

    ConsoleLogger::getInstance().onNewLogEntry = [this](const LogEntry& entry) {
        bool matches = true;
        if (!currentFilter.empty())
        {
            std::string lowerTag = entry.tag;
            std::string lowerMsg = entry.message;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
            matches = (lowerTag.find(currentFilter) != std::string::npos || lowerMsg.find(currentFilter) != std::string::npos);
        }
        if (matches)
        {
            displayedEntries.push_back(entry);
            logListBox.updateContent();
            if (autoScrollEnabled && displayedEntries.size() > 0)
            {
                logListBox.scrollToEnsureRowIsOnscreen(static_cast<int>(displayedEntries.size()) - 1);
            }
            logListBox.repaint();
        }
    };

    ConsoleLogger::getInstance().onLogsCleared = [this]() {
        displayedEntries.clear();
        logListBox.updateContent();
        logListBox.repaint();
    };

    updateFilteredList();
}

ConsolePanelComponent::~ConsolePanelComponent()
{
    ConsoleLogger::getInstance().onNewLogEntry = nullptr;
    ConsoleLogger::getInstance().onLogsCleared = nullptr;
}

void ConsolePanelComponent::paint(juce::Graphics& g)
{
    g.fillAll(CarbonGoldLookAndFeel::slatePanel.darker(0.3f));

    // Top border line
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.5f));
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    // Toolbar background
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, 1, getWidth(), 28);
}

void ConsolePanelComponent::resized()
{
    int w = getWidth();
    int h = getHeight();

    titleLabel.setBounds(10, 4, 240, 22);

    int btnX = 260;
    searchFilter.setBounds(btnX, 4, 160, 22); btnX += 168;
    autoScrollToggle.setBounds(btnX, 4, 90, 22); btnX += 95;
    copyButton.setBounds(btnX, 4, 70, 22); btnX += 76;
    clearButton.setBounds(btnX, 4, 60, 22); btnX += 66;

    closeButton.setBounds(w - 28, 4, 22, 22);

    logListBox.setBounds(2, 30, w - 4, std::max(20, h - 32));
}

int ConsolePanelComponent::getNumRows()
{
    return static_cast<int>(displayedEntries.size());
}

void ConsolePanelComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(displayedEntries.size())) return;

    if (rowIsSelected)
    {
        g.fillAll(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.18f));
    }
    else if (rowNumber % 2 == 1)
    {
        g.fillAll(juce::Colours::white.withAlpha(0.015f));
    }

    const auto& entry = displayedEntries[static_cast<size_t>(rowNumber)];

    juce::String timeStr = "[" + entry.timestamp.formatted("%H:%M:%S.") + juce::String::formatted("%03d", entry.timestamp.getMilliseconds()) + "]";
    
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::plain));

    // Draw timestamp
    g.setColour(juce::Colours::grey);
    g.drawText(timeStr, 8, 0, 95, height, juce::Justification::centredLeft, true);

    // Draw tag
    juce::Colour tagColor = CarbonGoldLookAndFeel::goldAccent;
    if (entry.level == LogLevel::AudioProbe) tagColor = CarbonGoldLookAndFeel::cyberCyan;
    else if (entry.level == LogLevel::Warning) tagColor = juce::Colours::orange;
    else if (entry.level == LogLevel::Error) tagColor = juce::Colours::red;

    g.setColour(tagColor);
    juce::String tagDisplay = "[" + juce::String(entry.tag) + "]";
    g.drawText(tagDisplay, 106, 0, 110, height, juce::Justification::centredLeft, true);

    // Draw message
    juce::Colour msgColor = juce::Colours::lightgrey;
    if (entry.level == LogLevel::AudioProbe) msgColor = CarbonGoldLookAndFeel::cyberCyan.brighter(0.2f);
    else if (entry.level == LogLevel::Warning) msgColor = juce::Colours::gold;

    g.setColour(msgColor);
    g.drawText(juce::String(entry.message), 220, 0, width - 230, height, juce::Justification::centredLeft, true);
}

void ConsolePanelComponent::updateFilteredList()
{
    displayedEntries.clear();
    auto all = ConsoleLogger::getInstance().getAllEntries();

    for (const auto& e : all)
    {
        if (currentFilter.empty())
        {
            displayedEntries.push_back(e);
        }
        else
        {
            std::string lowerTag = e.tag;
            std::string lowerMsg = e.message;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), ::tolower);
            if (lowerTag.find(currentFilter) != std::string::npos || lowerMsg.find(currentFilter) != std::string::npos)
            {
                displayedEntries.push_back(e);
            }
        }
    }
    logListBox.updateContent();
    if (autoScrollEnabled && displayedEntries.size() > 0)
    {
        logListBox.scrollToEnsureRowIsOnscreen(static_cast<int>(displayedEntries.size()) - 1);
    }
    logListBox.repaint();
}

void ConsolePanelComponent::clearLogs()
{
    ConsoleLogger::getInstance().clear();
}

void ConsolePanelComponent::copyAllLogsToClipboard()
{
    juce::String text = "";
    for (const auto& e : displayedEntries)
    {
        text += e.getFormattedString() + "\n";
    }
    juce::SystemClipboard::copyTextToClipboard(text);
}

void ConsolePanelComponent::refreshLogs()
{
    updateFilteredList();
}

} // namespace TimeDilationDAW
