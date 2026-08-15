#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../dsp/RelativisticSoundNodes.h"
#include <functional>
#include <vector>
#include <memory>

namespace TimeDilationDAW
{

// Individual sample row item card
class SampleItemComponent : public juce::Component
{
public:
    SampleItemComponent(const std::string& tableName,
                        std::function<void(const std::string&)> onAuditionRequested,
                        std::function<void(const std::string&)> onSpawnTabPlay,
                        std::function<void(const std::string&)> onSpawnTabRead4,
                        std::function<void(const std::string&)> onRemoveRequested);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setIsAuditioning(bool isPlaying);
    const std::string& getTableName() const { return name; }

private:
    std::string name;
    bool isAuditioning = false;

    juce::Label nameLabel;
    juce::Label metaLabel;
    juce::TextButton auditionButton{ "▶" };
    juce::TextButton spawnPlayButton{ "+ tabplay~" };
    juce::TextButton spawnWavetableButton{ "+ tabread4~" };
    juce::TextButton removeButton{ "🗑" };

    std::function<void(const std::string&)> auditionCallback;
    std::function<void(const std::string&)> spawnTabPlayCallback;
    std::function<void(const std::string&)> spawnTabRead4Callback;
    std::function<void(const std::string&)> removeCallback;
};

// Main Sample Pool Component
class SamplePoolComponent : public juce::Component,
                            public juce::FileDragAndDropTarget,
                            public TableManager::Listener,
                            public juce::Timer
{
public:
    using SpawnNodeCallback = std::function<void(const std::string& nodeSymbol, int x, int y)>;

    SamplePoolComponent();
    ~SamplePoolComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // File Drag and Drop Target
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // TableManager Listener
    void onTablePoolChanged() override;

    // Timer for UI refresh & Audition playback
    void timerCallback() override;

    void setSpawnNodeCallback(SpawnNodeCallback cb) { spawnNodeCallback = std::move(cb); }

    void importFilesWithChooser();
    void loadAudioFiles(const juce::StringArray& filePaths);
    void playAudition(const std::string& tableName);
    void stopAudition();

private:
    void rebuildSampleList();

    bool isDraggingOver = false;
    SpawnNodeCallback spawnNodeCallback;

    juce::Label headerTitle;
    juce::TextButton importButton{ "➕ Import Audio File..." };
    juce::TextButton clearAllButton{ "Clear Pool" };

    juce::Viewport viewport;
    std::unique_ptr<juce::Component> listContainer;
    std::vector<std::unique_ptr<SampleItemComponent>> sampleItems;

    // Audition preview playback
    std::string currentAuditionTable;
    bool isAuditionPlaying = false;
    double auditionPlayhead = 0.0;
    double auditionSampleRate = 44100.0;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace TimeDilationDAW
