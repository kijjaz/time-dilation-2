#include "SamplePoolComponent.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace TimeDilationDAW
{

// ============================================================================
// SampleItemComponent Implementation
// ============================================================================

SampleItemComponent::SampleItemComponent(const std::string& tableName,
                                         std::function<void(const std::string&)> onAuditionRequested,
                                         std::function<void(const std::string&)> onSpawnTabPlay,
                                         std::function<void(const std::string&)> onSpawnTabRead4,
                                         std::function<void(const std::string&)> onSpawnTabWrite,
                                         std::function<void(const std::string&)> onRemoveRequested)
    : name(tableName)
    , auditionCallback(std::move(onAuditionRequested))
    , spawnTabPlayCallback(std::move(onSpawnTabPlay))
    , spawnTabRead4Callback(std::move(onSpawnTabRead4))
    , spawnTabWriteCallback(std::move(onSpawnTabWrite))
    , removeCallback(std::move(onRemoveRequested))
{
    // Name Label (Editable on double-click)
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe6c66e)); // Warm Gold
    nameLabel.setEditable(true);
    nameLabel.onTextChange = [this]() {
        juce::String newName = nameLabel.getText().trim();
        if (newName.isNotEmpty() && newName.toStdString() != name)
        {
            TableManager::getInstance().renameTable(name, newName.toStdString());
            name = newName.toStdString();
        }
    };
    addAndMakeVisible(nameLabel);

    // Meta Label (sample rate, channels, duration)
    metaLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    metaLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a9ba8));
    const auto* info = TableManager::getInstance().getTableInfo(name);
    if (info)
    {
        juce::String chStr = (info->numChannels > 1) ? "Stereo" : "Mono";
        juce::String durStr = juce::String::formatted("%.2fs", info->durationSec);
        juce::String srStr = juce::String::formatted("%.1fkHz", info->sampleRate / 1000.0);
        metaLabel.setText(durStr + " | " + srStr + " | " + chStr, juce::dontSendNotification);
    }
    else
    {
        metaLabel.setText("Table Array", juce::dontSendNotification);
    }
    addAndMakeVisible(metaLabel);

    // Audition Button
    auditionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff23262d));
    auditionButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd4af37));
    auditionButton.onClick = [this]() {
        if (auditionCallback) auditionCallback(name);
    };
    addAndMakeVisible(auditionButton);

    // Spawn tabplay~ button
    spawnPlayButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1f2838));
    spawnPlayButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff64b5f6)); // Light cyan
    spawnPlayButton.onClick = [this]() {
        if (spawnTabPlayCallback) spawnTabPlayCallback(name);
    };
    addAndMakeVisible(spawnPlayButton);

    // Spawn tabread4~ button
    spawnWavetableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff281e38));
    spawnWavetableButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffba68c8)); // Light violet
    spawnWavetableButton.onClick = [this]() {
        if (spawnTabRead4Callback) spawnTabRead4Callback(name);
    };
    addAndMakeVisible(spawnWavetableButton);

    // Spawn tabwrite~ button
    spawnRecordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff381f20));
    spawnRecordButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff8a80)); // Light red/coral
    spawnRecordButton.onClick = [this]() {
        if (spawnTabWriteCallback) spawnTabWriteCallback(name);
    };
    addAndMakeVisible(spawnRecordButton);

    // Remove Button
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d1e20));
    removeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe57373)); // Soft red
    removeButton.onClick = [this]() {
        if (removeCallback) removeCallback(name);
    };
    addAndMakeVisible(removeButton);
}

void SampleItemComponent::setIsAuditioning(bool isPlaying)
{
    isAuditioning = isPlaying;
    auditionButton.setButtonText(isPlaying ? "⏹" : "▶");
    auditionButton.setColour(juce::TextButton::buttonColourId, isPlaying ? juce::Colour(0xff7a5c1e) : juce::Colour(0xff23262d));
    repaint();
}

void SampleItemComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 6)
    {
        if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            dragContainer->startDragging(juce::var(juce::String("sample:" + name)), this);
        }
    }
}

void SampleItemComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // Card background
    g.setColour(juce::Colour(0xff1a1c22));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Outline
    g.setColour(isAuditioning ? juce::Colour(0xffd4af37) : juce::Colour(0xff2e323b));
    g.drawRoundedRectangle(bounds, 6.0f, isAuditioning ? 1.5f : 1.0f);

    // Waveform Thumbnail Box
    auto waveArea = bounds.removeFromLeft(120.0f).reduced(4.0f);
    g.setColour(juce::Colour(0xff121316));
    g.fillRoundedRectangle(waveArea, 4.0f);

    const auto* info = TableManager::getInstance().getTableInfo(name);
    if (info && !info->thumbnailPeaks.empty())
    {
        g.setColour(isAuditioning ? juce::Colour(0xffffd54f) : juce::Colour(0xffe6c66e).withAlpha(0.7f));
        const auto& peaks = info->thumbnailPeaks;
        float midY = waveArea.getCentreY();
        float halfH = waveArea.getHeight() * 0.45f;
        float stepX = waveArea.getWidth() / static_cast<float>(peaks.size());

        for (size_t i = 0; i < peaks.size(); ++i)
        {
            float px = waveArea.getX() + static_cast<float>(i) * stepX;
            float h = std::clamp(peaks[i] * halfH, 1.0f, halfH);
            g.drawLine(px, midY - h, px, midY + h, 1.0f);
        }
    }
    else
    {
        // Simple sine wave fallback preview for math tables
        g.setColour(juce::Colour(0xff4fc3f7).withAlpha(0.6f));
        juce::Path p;
        float midY = waveArea.getCentreY();
        float amp = waveArea.getHeight() * 0.35f;
        p.startNewSubPath(waveArea.getX(), midY);
        for (float x = 0; x < waveArea.getWidth(); x += 2.0f)
        {
            float norm = x / waveArea.getWidth();
            float y = midY - std::sin(norm * 6.28318530718f) * amp;
            p.lineTo(waveArea.getX() + x, y);
        }
        g.strokePath(p, juce::PathStrokeType(1.2f));
    }
}

void SampleItemComponent::resized()
{
    auto area = getLocalBounds().reduced(6);
    area.removeFromLeft(124); // Space for waveform thumbnail

    auto topRow = area.removeFromTop(20);
    nameLabel.setBounds(topRow.removeFromLeft(160));
    metaLabel.setBounds(topRow);

    area.removeFromTop(4);
    auto btnRow = area.removeFromTop(24);

    auditionButton.setBounds(btnRow.removeFromLeft(36));
    btnRow.removeFromLeft(6);
    spawnPlayButton.setBounds(btnRow.removeFromLeft(82));
    btnRow.removeFromLeft(6);
    spawnWavetableButton.setBounds(btnRow.removeFromLeft(88));
    btnRow.removeFromLeft(6);
    spawnRecordButton.setBounds(btnRow.removeFromLeft(84));
    btnRow.removeFromLeft(6);
    removeButton.setBounds(btnRow.removeFromRight(32));
}


// ============================================================================
// SamplePoolComponent Implementation
// ============================================================================

SamplePoolComponent::SamplePoolComponent()
{
    headerTitle.setText("📁 Audio Sample Pool & Wavetables", juce::dontSendNotification);
    headerTitle.setFont(juce::Font(16.0f, juce::Font::bold));
    headerTitle.setColour(juce::Label::textColourId, juce::Colour(0xfff0d078));
    addAndMakeVisible(headerTitle);

    importButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a3442));
    importButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd4af37));
    importButton.onClick = [this]() { importFilesWithChooser(); };
    addAndMakeVisible(importButton);

    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22252c));
    clearAllButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff9e9e9e));
    clearAllButton.onClick = [this]() {
        auto names = TableManager::getInstance().getAllTableNames();
        for (const auto& n : names)
        {
            if (n != "sine") TableManager::getInstance().removeTable(n);
        }
    };
    addAndMakeVisible(clearAllButton);

    listContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(listContainer.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    TableManager::getInstance().addListener(this);
    rebuildSampleList();

    startTimerHz(30); // 30Hz refresh timer for smooth audition tracking
}

SamplePoolComponent::~SamplePoolComponent()
{
    stopTimer();
    TableManager::getInstance().removeListener(this);
}

void SamplePoolComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121316)); // Deep carbon background

    // Header bar separator line
    g.setColour(juce::Colour(0xff262a33));
    g.drawLine(0.0f, 48.0f, static_cast<float>(getWidth()), 48.0f, 1.0f);

    // Drag-and-drop feedback overlay
    if (isDraggingOver)
    {
        auto dropArea = viewport.getBounds().toFloat().reduced(8.0f);
        g.setColour(juce::Colour(0xffd4af37).withAlpha(0.15f));
        g.fillRoundedRectangle(dropArea, 8.0f);

        g.setColour(juce::Colour(0xffd4af37));
        float dashLengths[] = { 6.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(dropArea.getTopLeft(), dropArea.getTopRight()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropArea.getTopRight(), dropArea.getBottomRight()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropArea.getBottomRight(), dropArea.getBottomLeft()), dashLengths, 2, 2.0f);
        g.drawDashedLine(juce::Line<float>(dropArea.getBottomLeft(), dropArea.getTopLeft()), dashLengths, 2, 2.0f);

        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("Release to Import Audio Samples into Pool", dropArea, juce::Justification::centred);
    }
    else if (sampleItems.empty())
    {
        // Empty state banner
        auto emptyArea = viewport.getBounds().toFloat().reduced(20.0f);
        g.setColour(juce::Colour(0xff8a9ba8).withAlpha(0.5f));
        g.setFont(juce::Font(14.0f, juce::Font::italic));
        g.drawText("No audio samples in pool.\nDrag and drop .wav, .aif, .flac, or .mp3 files here,\nor click 'Import Audio File...'",
                   emptyArea, juce::Justification::centred);
    }
}

void SamplePoolComponent::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(48).reduced(8, 6);

    headerTitle.setBounds(header.removeFromLeft(280));
    clearAllButton.setBounds(header.removeFromRight(90));
    header.removeFromRight(8);
    importButton.setBounds(header.removeFromRight(160));

    viewport.setBounds(area);

    if (listContainer)
    {
        int itemH = 64;
        int spacing = 4;
        int totalH = static_cast<int>(sampleItems.size()) * (itemH + spacing) + 8;
        listContainer->setSize(viewport.getWidth() - 14, std::max(totalH, viewport.getHeight()));

        int y = 4;
        for (auto& item : sampleItems)
        {
            item->setBounds(4, y, listContainer->getWidth() - 8, itemH);
            y += itemH + spacing;
        }
    }
}

bool SamplePoolComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        juce::File f(file);
        juce::String ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".ogg" || ext == ".mp3")
        {
            return true;
        }
    }
    return false;
}

void SamplePoolComponent::fileDragEnter(const juce::StringArray&, int, int)
{
    isDraggingOver = true;
    repaint();
}

void SamplePoolComponent::fileDragExit(const juce::StringArray&)
{
    isDraggingOver = false;
    repaint();
}

void SamplePoolComponent::filesDropped(const juce::StringArray& files, int, int)
{
    isDraggingOver = false;
    loadAudioFiles(files);
    repaint();
}

void SamplePoolComponent::onTablePoolChanged()
{
    rebuildSampleList();
}

void SamplePoolComponent::timerCallback()
{
    // Handle Audition playback
    if (isAuditionPlaying && !currentAuditionTable.empty())
    {
        const auto* info = TableManager::getInstance().getTableInfo(currentAuditionTable);
        if (info && info->durationSec > 0.0)
        {
            auditionPlayhead += 1.0 / 30.0;
            if (auditionPlayhead >= info->durationSec)
            {
                stopAudition();
            }
        }
        else
        {
            stopAudition();
        }
    }
}

void SamplePoolComponent::importFilesWithChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio Files to Import into Sample Pool...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectFiles |
                        juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto results = fc.getResults();
        juce::StringArray paths;
        for (const auto& file : results)
        {
            paths.add(file.getFullPathName());
        }
        if (!paths.isEmpty())
        {
            loadAudioFiles(paths);
        }
    });
}

void SamplePoolComponent::loadAudioFiles(const juce::StringArray& filePaths)
{
    for (const auto& path : filePaths)
    {
        juce::File file(path);
        if (file.existsAsFile())
        {
            juce::String baseName = file.getFileNameWithoutExtension();
            // Sanitize name for clean table symbol usage (alphanumeric and underscore)
            juce::String cleanName = baseName.replaceCharacter(' ', '_').replaceCharacter('-', '_');

            // Avoid collisions
            std::string finalName = cleanName.toStdString();
            int count = 1;
            while (TableManager::getInstance().hasTable(finalName))
            {
                finalName = cleanName.toStdString() + "_" + std::to_string(count++);
            }

            TableManager::getInstance().loadSample(finalName, file);
        }
    }
}

void SamplePoolComponent::playAudition(const std::string& tableName)
{
    if (isAuditionPlaying && currentAuditionTable == tableName)
    {
        stopAudition();
        return;
    }

    currentAuditionTable = tableName;
    isAuditionPlaying = true;
    auditionPlayhead = 0.0;

    for (auto& item : sampleItems)
    {
        item->setIsAuditioning(item->getTableName() == tableName);
    }
}

void SamplePoolComponent::stopAudition()
{
    isAuditionPlaying = false;
    currentAuditionTable = "";
    for (auto& item : sampleItems)
    {
        item->setIsAuditioning(false);
    }
}

void SamplePoolComponent::rebuildSampleList()
{
    sampleItems.clear();
    if (!listContainer) return;

    listContainer->removeAllChildren();

    auto tableNames = TableManager::getInstance().getAllTableNames();
    for (const auto& name : tableNames)
    {
        auto item = std::make_unique<SampleItemComponent>(
            name,
            [this](const std::string& n) { playAudition(n); },
            [this](const std::string& n) {
                if (spawnNodeCallback) spawnNodeCallback("tabplay~ " + n, 250, 200);
            },
            [this](const std::string& n) {
                if (spawnNodeCallback) spawnNodeCallback("tabread4~ " + n, 250, 200);
            },
            [this](const std::string& n) {
                if (spawnNodeCallback) spawnNodeCallback("tabwrite~ " + n, 250, 200);
            },
            [](const std::string& n) {
                TableManager::getInstance().removeTable(n);
            }
        );

        listContainer->addAndMakeVisible(item.get());
        sampleItems.push_back(std::move(item));
    }

    resized();
    repaint();
}

} // namespace TimeDilationDAW
