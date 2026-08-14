#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <iostream>

namespace TimeDilationDAW
{

enum class LogLevel
{
    Message,
    AudioProbe,
    System,
    Warning,
    Error
};

struct LogEntry
{
    juce::Time timestamp;
    std::string tag;
    std::string message;
    LogLevel level = LogLevel::Message;

    juce::String getFormattedString() const
    {
        juce::String timeStr = timestamp.formatted("%H:%M:%S.") + juce::String::formatted("%03d", timestamp.getMilliseconds());
        juce::String lvlPrefix = "";
        if (level == LogLevel::AudioProbe) lvlPrefix = "[PROBE~] ";
        else if (level == LogLevel::Warning) lvlPrefix = "[WARN] ";
        else if (level == LogLevel::Error) lvlPrefix = "[ERROR] ";

        if (!tag.empty())
        {
            return "[" + timeStr + "] " + lvlPrefix + "[" + juce::String(tag) + "] " + juce::String(message);
        }
        return "[" + timeStr + "] " + lvlPrefix + juce::String(message);
    }
};

class ConsoleLogger
{
public:
    static ConsoleLogger& getInstance()
    {
        static ConsoleLogger instance;
        return instance;
    }

    void log(const std::string& message, const std::string& tag = "print", LogLevel level = LogLevel::Message)
    {
        LogEntry entry{ juce::Time::getCurrentTime(), tag, message, level };
        
        {
            const std::lock_guard<std::mutex> lock(mutex);
            entries.push_back(entry);
            if (entries.size() > maxEntries)
            {
                entries.erase(entries.begin(), entries.begin() + 100);
            }
        }

        // Output to standard console for agent and developer inspection
        std::cout << entry.getFormattedString().toStdString() << "\n";

        // Notify UI listener
        if (onNewLogEntry)
        {
            juce::MessageManager::callAsync([this, entry]() {
                if (onNewLogEntry) onNewLogEntry(entry);
            });
        }
    }

    void logProbe(const std::string& tag, float peakDb, float rmsDb, float envelope, bool active)
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Peak: %+.1f dBFS | RMS: %+.1f dBFS | Env: %.2f | Active: %s",
                      peakDb, rmsDb, envelope, active ? "YES" : "SILENT");
        log(buf, tag, LogLevel::AudioProbe);
    }

    std::vector<LogEntry> getAllEntries() const
    {
        const std::lock_guard<std::mutex> lock(mutex);
        return entries;
    }

    void clear()
    {
        const std::lock_guard<std::mutex> lock(mutex);
        entries.clear();
        if (onLogsCleared)
        {
            juce::MessageManager::callAsync([this]() {
                if (onLogsCleared) onLogsCleared();
            });
        }
    }

    std::function<void(const LogEntry&)> onNewLogEntry;
    std::function<void()> onLogsCleared;

private:
    ConsoleLogger() = default;
    mutable std::mutex mutex;
    std::vector<LogEntry> entries;
    size_t maxEntries = 2000;
};

} // namespace TimeDilationDAW
