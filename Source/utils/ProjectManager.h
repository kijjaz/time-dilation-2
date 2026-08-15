#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <string>
#include <functional>

namespace TimeDilationDAW
{

class ProjectManager
{
public:
    static ProjectManager& getInstance()
    {
        static ProjectManager instance;
        return instance;
    }

    // Initialize or reset to a clean default temporary session in system temp
    void initializeDefaultSession();

    // Set active project file (and updates project directory to file's parent directory)
    void setProjectFile(const juce::File& file);

    // Set active project directory directly
    void setProjectDirectory(const juce::File& dir);

    // Returns whether the project is currently saved to a real user directory or running in temp
    bool isProjectSaved() const { return isSavedProject; }

    const juce::File& getProjectFile() const { return currentProjectFile; }
    const juce::File& getProjectDirectory() const { return currentProjectDir; }

    // Subdirectories (guaranteed to exist upon calling)
    juce::File getAudioDirectory() const;
    juce::File getRecordingsDirectory() const;
    juce::File getBouncesDirectory() const;

    // Resolve an audio file path: checks ./audio/<path>, ./<path>, and absolute paths
    juce::File resolveAudioFile(const juce::String& pathOrName) const;

    // Copy or write all current session audio assets into the target project's ./audio/ folder
    void syncSessionAssetsToProject();

private:
    ProjectManager();
    ~ProjectManager() = default;

    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    juce::File currentProjectFile;
    juce::File currentProjectDir;
    juce::File tempSessionDir;
    bool isSavedProject = false;
};

} // namespace TimeDilationDAW
