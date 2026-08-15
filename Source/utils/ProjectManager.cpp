#include "ProjectManager.h"
#include "../dsp/RelativisticSoundNodes.h"
#include "ConsoleLogger.h"

namespace TimeDilationDAW
{

ProjectManager::ProjectManager()
{
    initializeDefaultSession();
}

void ProjectManager::initializeDefaultSession()
{
    // Use OS default temporary directory for unsaved live sessions
    tempSessionDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("TimeDilationDAW")
                        .getChildFile("Session");

    tempSessionDir.createDirectory();
    currentProjectDir = tempSessionDir;
    currentProjectFile = juce::File{};
    isSavedProject = false;

    getAudioDirectory();
    getRecordingsDirectory();
    getBouncesDirectory();
}

void ProjectManager::setProjectFile(const juce::File& file)
{
    currentProjectFile = file;
    if (currentProjectFile != juce::File{})
    {
        currentProjectDir = currentProjectFile.getParentDirectory();
        currentProjectDir.createDirectory();
        isSavedProject = true;

        getAudioDirectory();
        getRecordingsDirectory();
        getBouncesDirectory();
    }
    else
    {
        initializeDefaultSession();
    }
}

void ProjectManager::setProjectDirectory(const juce::File& dir)
{
    currentProjectDir = dir;
    currentProjectDir.createDirectory();
    isSavedProject = true;

    getAudioDirectory();
    getRecordingsDirectory();
    getBouncesDirectory();
}

juce::File ProjectManager::getAudioDirectory() const
{
    auto audioDir = currentProjectDir.getChildFile("audio");
    if (!audioDir.isDirectory())
    {
        audioDir.createDirectory();
    }
    return audioDir;
}

juce::File ProjectManager::getRecordingsDirectory() const
{
    auto recDir = currentProjectDir.getChildFile("recordings");
    if (!recDir.isDirectory())
    {
        recDir.createDirectory();
    }
    return recDir;
}

juce::File ProjectManager::getBouncesDirectory() const
{
    auto bounceDir = currentProjectDir.getChildFile("bounces");
    if (!bounceDir.isDirectory())
    {
        bounceDir.createDirectory();
    }
    return bounceDir;
}

juce::File ProjectManager::resolveAudioFile(const juce::String& pathOrName) const
{
    juce::String cleanPath = pathOrName.trim();
    if (cleanPath.isEmpty()) return juce::File{};

    // 1. Check if absolute path exists
    juce::File directFile(cleanPath);
    if (directFile.existsAsFile())
    {
        return directFile;
    }

    // 2. Check ./audio/<pathOrName> inside project directory
    auto inAudio = getAudioDirectory().getChildFile(cleanPath);
    if (inAudio.existsAsFile())
    {
        return inAudio;
    }

    // 3. Check directly in project directory ./<pathOrName>
    auto inProj = currentProjectDir.getChildFile(cleanPath);
    if (inProj.existsAsFile())
    {
        return inProj;
    }

    // 4. Check current working directory
    auto inCwd = juce::File::getCurrentWorkingDirectory().getChildFile(cleanPath);
    if (inCwd.existsAsFile())
    {
        return inCwd;
    }

    // Return target in ./audio/ by default so writers know where to place it
    return getAudioDirectory().getChildFile(cleanPath);
}

void ProjectManager::syncSessionAssetsToProject()
{
    if (!isSavedProject) return;

    auto targetAudioDir = getAudioDirectory();

    // 1. Copy any files from tempSessionDir/audio into target project audio folder if different
    if (tempSessionDir.isDirectory() && tempSessionDir != currentProjectDir)
    {
        auto tempAudio = tempSessionDir.getChildFile("audio");
        if (tempAudio.isDirectory())
        {
            auto files = tempAudio.findChildFiles(juce::File::findFiles, false);
            for (const auto& src : files)
            {
                auto dest = targetAudioDir.getChildFile(src.getFileName());
                if (!dest.existsAsFile() || dest.getLastModificationTime() < src.getLastModificationTime())
                {
                    src.copyFileTo(dest);
                }
            }
        }
    }

    // 2. Write all active tables in TableManager as WAV files into ./audio/
    TableManager::getInstance().saveAllTablesToDirectory(targetAudioDir);
}

} // namespace TimeDilationDAW
