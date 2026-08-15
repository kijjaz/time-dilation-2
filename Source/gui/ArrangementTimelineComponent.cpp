#include "ArrangementTimelineComponent.h"
#include "CarbonGoldLookAndFeel.h"
#include "../dsp/TidalSeqNode.h"
#include <iomanip>
#include <sstream>

namespace TimeDilationDAW
{

ArrangementTimelineComponent::ArrangementTimelineComponent(RelativisticNodeGraph& graph)
    : nodeGraph(graph)
{
    for (int t = 0; t < 8; ++t)
    {
        TimelineTrackInfo info;
        info.trackIndex = t;
        info.name = "Track " + juce::String(t + 1);
        info.defaultClipType = (t % 2 == 0) ? ClipType::Pattern : ClipType::AudioSample;
        info.isArmed = false;
        info.inputSource.type = (t == 1) ? AudioInputType::ExternalMono : (t == 2 ? AudioInputType::InternalMaster : AudioInputType::ExternalStereo);
        info.inputSource.primaryChannel = 1;
        info.inputSource.secondaryChannel = 2;
        tracks.push_back(info);
    }

    playStopButton.onClick = [this]() { togglePlayback(); };
    playStopButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    playStopButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(playStopButton);

    rewindButton.onClick = [this]() { rewindToStart(); };
    rewindButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    rewindButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(rewindButton);

    loopButton.onClick = [this]() {
        isLoopEnabled = !isLoopEnabled;
        loopButton.setButtonText(isLoopEnabled ? "LOOP ON" : "LOOP OFF");
        repaint();
    };
    loopButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    loopButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::royalViolet);
    addAndMakeVisible(loopButton);

    addClipButton.onClick = [this]() {
        addClip(0, playheadTimeSec, 4.0, "Pattern Clip", ClipType::Pattern);
    };
    addClipButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    addClipButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(addClipButton);

    addTrackButton.onClick = [this]() {
        addTrack("Track " + juce::String(tracks.size() + 1));
    };
    addTrackButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    addTrackButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(addTrackButton);

    recQuantizeButton.onClick = [this]() { showRecordingQuantizeMenu(); };
    recQuantizeButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    recQuantizeButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(recQuantizeButton);

    trackRenameEditor.setFont(juce::Font(11.0f, juce::Font::bold));
    trackRenameEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
    trackRenameEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    trackRenameEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::cyberCyan);
    trackRenameEditor.onReturnKey = [this]() {
        if (renamingTrackIdx >= 0 && renamingTrackIdx < static_cast<int>(tracks.size()))
        {
            renameTrack(renamingTrackIdx, trackRenameEditor.getText().trim());
        }
        trackRenameEditor.setVisible(false);
        renamingTrackIdx = -1;
    };
    trackRenameEditor.onFocusLost = [this]() {
        if (renamingTrackIdx >= 0 && renamingTrackIdx < static_cast<int>(tracks.size()))
        {
            renameTrack(renamingTrackIdx, trackRenameEditor.getText().trim());
        }
        trackRenameEditor.setVisible(false);
        renamingTrackIdx = -1;
    };
    addChildComponent(trackRenameEditor);

    togglePianoRollBtn.onClick = [this]() {
        isPianoRollVisible = !isPianoRollVisible;
        togglePianoRollBtn.setColour(juce::TextButton::textColourOffId, isPianoRollVisible ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::grey);
        resized();
        repaint();
    };
    togglePianoRollBtn.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    togglePianoRollBtn.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(togglePianoRollBtn);

    timeDisplayLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    timeDisplayLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(timeDisplayLabel);

    // Event Text Editor Setup
    eventEditor.setFont(juce::Font(11.0f, juce::Font::bold));
    eventEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
    eventEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    eventEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::goldAccent);
    eventEditor.onReturnKey = [this]() { commitEventEditor(); };
    eventEditor.onFocusLost = [this]() { commitEventEditor(); };
    eventEditor.onEscapeKey = [this]() { eventEditor.setVisible(false); isEditingEvent = false; };
    addChildComponent(eventEditor);

    // Inline In-Place Block Text Editor Setup
    inlineBlockEditor.setFont(juce::Font(12.0f, juce::Font::bold));
    inlineBlockEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::carbonBg);
    inlineBlockEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    inlineBlockEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::cyberCyan);
    inlineBlockEditor.onReturnKey = [this]() { commitInlineBlockEditing(); };
    inlineBlockEditor.onFocusLost = [this]() { commitInlineBlockEditing(); };
    inlineBlockEditor.onEscapeKey = [this]() { inlineBlockEditor.setVisible(false); isInlineEditingBlock = false; };
    addChildComponent(inlineBlockEditor);

    // Tidal Pattern Editor & Quick Transformation Macros
    tidalPatternEditor.setFont(juce::Font(12.0f, juce::Font::bold));
    tidalPatternEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.4f));
    tidalPatternEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    tidalPatternEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::cyberCyan);
    tidalPatternEditor.setText("[60 [62 64] 67 [69 71 72]]");
    tidalPatternEditor.onReturnKey = [this]() { commitTidalPattern(); };
    addAndMakeVisible(tidalPatternEditor);

    auto setupMacroBtn = [this](juce::TextButton& btn, juce::Colour col) {
        btn.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.08f));
        btn.setColour(juce::TextButton::textColourOffId, col);
        addAndMakeVisible(btn);
    };

    wrapBracketBtn.onClick = [this]() { wrapSelectedBlockInBrackets(); };
    setupMacroBtn(wrapBracketBtn, CarbonGoldLookAndFeel::goldAccent);

    subdivideBtn.onClick = [this]() { subdivideSelectedBlock(2); };
    setupMacroBtn(subdivideBtn, CarbonGoldLookAndFeel::cyberCyan);

    tripletBtn.onClick = [this]() { subdivideSelectedBlock(3); };
    setupMacroBtn(tripletBtn, CarbonGoldLookAndFeel::cyberCyan);

    quadBtn.onClick = [this]() { subdivideSelectedBlock(4); };
    setupMacroBtn(quadBtn, CarbonGoldLookAndFeel::cyberCyan);

    unwrapBtn.onClick = [this]() { unwrapSelectedBlock(); };
    setupMacroBtn(unwrapBtn, CarbonGoldLookAndFeel::cyberCyan);

    stackBtn.onClick = [this]() { showAddStackMenu(); };
    setupMacroBtn(stackBtn, CarbonGoldLookAndFeel::goldAccent);

    euclidBtn.onClick = [this]() { applyEuclideanMacro(3, 8); };
    setupMacroBtn(euclidBtn, juce::Colour(0xffff9900));

    alternateBtn.onClick = [this]() { applyAlternateMacro(); };
    setupMacroBtn(alternateBtn, CarbonGoldLookAndFeel::royalViolet);

    speed2Btn.onClick = [this]() { applySpeedMacro(2.0); };
    setupMacroBtn(speed2Btn, CarbonGoldLookAndFeel::cyberCyan);

    degradeBtn.onClick = [this]() { applyDegradeMacro(); };
    setupMacroBtn(degradeBtn, juce::Colours::pink);

    restBtn.onClick = [this]() { toggleSelectedBlockRest(); };
    setupMacroBtn(restBtn, juce::Colours::grey);

    pitchUpBtn.onClick = [this]() { shiftSelectedBlockPitch(1); };
    setupMacroBtn(pitchUpBtn, CarbonGoldLookAndFeel::goldAccent);

    pitchDownBtn.onClick = [this]() { shiftSelectedBlockPitch(-1); };
    setupMacroBtn(pitchDownBtn, CarbonGoldLookAndFeel::goldAccent);

    octUpBtn.onClick = [this]() { shiftSelectedBlockPitch(12); };
    setupMacroBtn(octUpBtn, CarbonGoldLookAndFeel::goldAccent);

    octDownBtn.onClick = [this]() { shiftSelectedBlockPitch(-12); };
    setupMacroBtn(octDownBtn, CarbonGoldLookAndFeel::goldAccent);

    applyPatternBtn.onClick = [this]() { commitTidalPattern(); };
    applyPatternBtn.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::goldAccent);
    applyPatternBtn.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::carbonBg);
    addAndMakeVisible(applyPatternBtn);

    tidalHelpBtn.onClick = [this]() { showTidalHelpModal(); };
    tidalHelpBtn.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.darker(0.2f));
    tidalHelpBtn.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(tidalHelpBtn);

    refreshTimeline();
    startTimerHz(30);
}

void ArrangementTimelineComponent::showTidalHelpModal()
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "TidalCycles Pattern Editor Guide",
        "TidalCycles Mini-Notation Cheat Sheet:\n\n"
        "1. Subdivisions [a b c]:\n"
        "   - [60 62 64 67] -> 4 quarter notes per cycle\n"
        "   - [60 [62 64] 67 [69 71 72]] -> Nested subdivisions\n\n"
        "2. Polyphonic Stacking [a, b]:\n"
        "   - [60 64 67, 36 [~ 48]] -> Parallel voices\n\n"
        "3. Euclidean (k, n, rot):\n"
        "   - 60(3,8) -> 3 pulses in 8 steps\n"
        "   - 36(5,16,2) -> 5 pulses in 16 steps, rotated by 2\n\n"
        "4. Speed Multipliers:\n"
        "   - 60*4 -> 4x speed\n"
        "   - [60 62]*2 -> Repeat twice\n\n"
        "5. Cycle Alternation:\n"
        "   - <60 62 65 67> -> Alternate each cycle\n\n"
        "6. Rests & Prob:\n"
        "   - ~ = rest, 60?0.8 = 80% prob\n\n"
        "7. Drum Aliases: bd, sn, cp, hh, ht, lt, mt, cb, rim, cl, rd, cr");
}

void ArrangementTimelineComponent::showTidalBlockContextMenu(TimelineClip& clip, int eventIdx)
{
    if (eventIdx < 0 || eventIdx >= static_cast<int>(clip.cachedEvents.size())) return;
    auto& ev = clip.cachedEvents[static_cast<size_t>(eventIdx)];
    selectedTidalEventIdx = eventIdx;
    selectedTidalChannel = ev.channel;

    juce::PopupMenu m;
    
    // 1. Bracket & Subdivision Actions
    juce::PopupMenu bracketMenu;
    bracketMenu.addItem(100, "[ ... ] Wrap in Brackets");
    bracketMenu.addItem(101, "[/2] Binary Subdivide [a b]");
    bracketMenu.addItem(102, "[/3] Triplet Subdivide [a b c]");
    bracketMenu.addItem(103, "[/4] Quad Subdivide [a b c d]");
    bracketMenu.addItem(104, "Unwrap / Flatten Brackets");
    m.addSubMenu("Brackets & Subdivisions", bracketMenu);

    // 2. Polyphonic Stacking
    juce::PopupMenu stackMenu;
    stackMenu.addItem(200, "+ Layer: Bassline (36 ~ 36 48)");
    stackMenu.addItem(201, "+ Layer: Hi-Hat Groove ([42 42] 42 [42 42] 46)");
    stackMenu.addItem(202, "+ Layer: Offbeat Snare (~ 38 ~ 38)");
    stackMenu.addItem(203, "+ Layer: Chord Stabs ([60 64 67])");
    stackMenu.addItem(204, "+ Layer: Custom Empty (~ ~ ~ ~)");
    stackMenu.addSeparator();
    stackMenu.addItem(205, "Delete Current Voice Layer (Channel " + juce::String(ev.channel + 1) + ")");
    m.addSubMenu("Polyphonic Stacks & Layers", stackMenu);

    m.addSeparator();

    // 3. Pitch & Note Manipulations
    m.addItem(1, "+1 Octave (Pitch: " + juce::String(ev.pitch + 12) + ")");
    m.addItem(2, "-1 Octave (Pitch: " + juce::String(std::max(12, ev.pitch - 12)) + ")");
    m.addItem(3, "+1 Semitone (Pitch: " + juce::String(ev.pitch + 1) + ")");
    m.addItem(4, "-1 Semitone (Pitch: " + juce::String(std::max(0, ev.pitch - 1)) + ")");
    m.addSeparator();

    // 4. Speed & Euclidean
    juce::PopupMenu subMenu;
    subMenu.addItem(13, "Speed *2");
    subMenu.addItem(14, "Speed *4");
    subMenu.addItem(15, "Speed /2");
    m.addSubMenu("Speed Modifiers", subMenu);

    juce::PopupMenu euclidMenu;
    euclidMenu.addItem(20, "Euclidean (3, 8) - Tresillo");
    euclidMenu.addItem(21, "Euclidean (5, 16) - Cinquillo");
    euclidMenu.addItem(22, "Euclidean (7, 16)");
    m.addSubMenu("Euclidean Generators", euclidMenu);

    juce::PopupMenu drumMenu;
    drumMenu.addItem(30, "Kick Drum (bd / 36)");
    drumMenu.addItem(31, "Snare Drum (sn / 38)");
    drumMenu.addItem(32, "Clap (cp / 39)");
    drumMenu.addItem(33, "Closed Hi-Hat (hh / 42)");
    drumMenu.addItem(34, "Open Hi-Hat (oh / 46)");
    drumMenu.addItem(35, "Cowbell (cb / 56)");
    drumMenu.addItem(36, "Rimshot (rim / 37)");
    m.addSubMenu("Drum Sounds", drumMenu);

    m.addSeparator();
    m.addItem(40, "Convert to Rest (~)");
    m.addItem(41, "Set 80% Probability (?0.8)");
    m.addItem(42, "Set 50% Probability (?0.5)");

    m.showMenuAsync(juce::PopupMenu::Options(), [this, &clip, eventIdx](int result) {
        if (eventIdx < 0 || eventIdx >= static_cast<int>(clip.cachedEvents.size())) return;
        auto& targetEv = clip.cachedEvents[static_cast<size_t>(eventIdx)];

        if (result == 100) { wrapSelectedBlockInBrackets(); return; }
        else if (result == 101) { subdivideSelectedBlock(2); return; }
        else if (result == 102) { subdivideSelectedBlock(3); return; }
        else if (result == 103) { subdivideSelectedBlock(4); return; }
        else if (result == 104) { unwrapSelectedBlock(); return; }
        else if (result == 200) { addStackLayer("36 ~ 36 48"); return; }
        else if (result == 201) { addStackLayer("[42 42] 42 [42 42] 46"); return; }
        else if (result == 202) { addStackLayer("~ 38 ~ 38"); return; }
        else if (result == 203) { addStackLayer("[60 64 67]"); return; }
        else if (result == 204) { addStackLayer("~ ~ ~ ~"); return; }
        else if (result == 205) { deleteStackLayer(targetEv.channel); return; }
        else if (result == 1) targetEv.pitch = std::min(127, targetEv.pitch + 12);
        else if (result == 2) targetEv.pitch = std::max(0, targetEv.pitch - 12);
        else if (result == 3) targetEv.pitch = std::min(127, targetEv.pitch + 1);
        else if (result == 4) targetEv.pitch = std::max(0, targetEv.pitch - 1);
        else if (result == 13) { targetEv.valueStr = (targetEv.valueStr.empty() ? juce::String(targetEv.pitch).toStdString() : targetEv.valueStr) + "*2"; }
        else if (result == 14) { targetEv.valueStr = (targetEv.valueStr.empty() ? juce::String(targetEv.pitch).toStdString() : targetEv.valueStr) + "*4"; }
        else if (result == 15) { targetEv.valueStr = (targetEv.valueStr.empty() ? juce::String(targetEv.pitch).toStdString() : targetEv.valueStr) + "/2"; }
        else if (result == 20) { targetEv.valueStr = juce::String(targetEv.pitch).toStdString() + "(3,8)"; }
        else if (result == 21) { targetEv.valueStr = juce::String(targetEv.pitch).toStdString() + "(5,16)"; }
        else if (result == 22) { targetEv.valueStr = juce::String(targetEv.pitch).toStdString() + "(7,16)"; }
        else if (result == 30) { targetEv.pitch = 36; targetEv.valueStr = "bd"; targetEv.isRest = false; }
        else if (result == 31) { targetEv.pitch = 38; targetEv.valueStr = "sn"; targetEv.isRest = false; }
        else if (result == 32) { targetEv.pitch = 39; targetEv.valueStr = "cp"; targetEv.isRest = false; }
        else if (result == 33) { targetEv.pitch = 42; targetEv.valueStr = "hh"; targetEv.isRest = false; }
        else if (result == 34) { targetEv.pitch = 46; targetEv.valueStr = "oh"; targetEv.isRest = false; }
        else if (result == 35) { targetEv.pitch = 56; targetEv.valueStr = "cb"; targetEv.isRest = false; }
        else if (result == 36) { targetEv.pitch = 37; targetEv.valueStr = "rim"; targetEv.isRest = false; }
        else if (result == 40) { targetEv.isRest = true; targetEv.valueStr = "~"; }
        else if (result == 41) { targetEv.valueStr = (targetEv.valueStr.empty() ? juce::String(targetEv.pitch).toStdString() : targetEv.valueStr) + "?0.8"; }
        else if (result == 42) { targetEv.valueStr = (targetEv.valueStr.empty() ? juce::String(targetEv.pitch).toStdString() : targetEv.valueStr) + "?0.5"; }

        // Rebuild full pattern string from events
        std::stringstream ss;
        ss << "[";
        int lastChan = -1;
        for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
        {
            const auto& e = clip.cachedEvents[i];
            if (lastChan >= 0 && e.channel != lastChan)
            {
                ss << ", ";
            }
            else if (i > 0)
            {
                ss << " ";
            }
            lastChan = e.channel;

            if (e.isRest) ss << "~";
            else if (!e.valueStr.empty()) ss << e.valueStr;
            else ss << e.pitch;
        }
        ss << "]";
        clip.updateTidalPattern(ss.str());
        tidalPatternEditor.setText(clip.tidalPattern);
        commitTidalPattern();
    });
}

void ArrangementTimelineComponent::wrapSelectedBlockInBrackets()
{
    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (selectedTidalEventIdx >= 0 && selectedTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(selectedTidalEventIdx)];
                std::string baseVal = ev.isRest ? "~" : (!ev.valueStr.empty() ? ev.valueStr : std::to_string(ev.pitch));
                ev.valueStr = "[" + baseVal + " " + baseVal + "]";
                ev.isRest = false;

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                return;
            }
        }
    }

    // Fallback: wrap whole pattern in brackets
    juce::String cur = tidalPatternEditor.getText().trim();
    if (!cur.startsWith("[")) cur = "[" + cur + "]";
    else cur = "[" + cur + " " + cur + "]";
    tidalPatternEditor.setText(cur);
    commitTidalPattern();
}

void ArrangementTimelineComponent::subdivideSelectedBlock(int count)
{
    count = std::clamp(count, 2, 8);
    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (selectedTidalEventIdx >= 0 && selectedTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(selectedTidalEventIdx)];
                int p = ev.pitch;
                std::stringstream subSS;
                subSS << "[";
                for (int s = 0; s < count; ++s)
                {
                    if (s > 0) subSS << " ";
                    if (ev.isRest) subSS << "~";
                    else if (!ev.valueStr.empty() && (ev.valueStr == "bd" || ev.valueStr == "sn" || ev.valueStr == "cp" || ev.valueStr == "hh"))
                        subSS << ev.valueStr;
                    else
                        subSS << (count == 3 ? (s == 0 ? p : (s == 1 ? p + 4 : p + 7)) : p + s * 2);
                }
                subSS << "]";
                ev.valueStr = subSS.str();
                ev.isRest = false;

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                return;
            }
        }
    }

    applySubdivisionMacro(count);
}

void ArrangementTimelineComponent::unwrapSelectedBlock()
{
    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (selectedTidalEventIdx >= 0 && selectedTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(selectedTidalEventIdx)];
                if (ev.valueStr.front() == '[' && ev.valueStr.back() == ']')
                {
                    ev.valueStr = ev.valueStr.substr(1, ev.valueStr.size() - 2);
                }
                else
                {
                    ev.valueStr = std::to_string(ev.pitch);
                }

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                return;
            }
        }
    }
}

void ArrangementTimelineComponent::shiftSelectedBlockPitch(int semitones)
{
    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (selectedTidalEventIdx >= 0 && selectedTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(selectedTidalEventIdx)];
                ev.pitch = std::clamp(ev.pitch + semitones, 0, 127);
                if (!ev.valueStr.empty() && std::isdigit(ev.valueStr[0]))
                {
                    ev.valueStr = std::to_string(ev.pitch);
                }
                ev.isRest = false;

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                return;
            }
        }
    }
}

void ArrangementTimelineComponent::toggleSelectedBlockRest()
{
    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (selectedTidalEventIdx >= 0 && selectedTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(selectedTidalEventIdx)];
                ev.isRest = !ev.isRest;
                if (ev.isRest) ev.valueStr = "~";
                else ev.valueStr = std::to_string(ev.pitch > 0 ? ev.pitch : 60);

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                return;
            }
        }
    }
}

void ArrangementTimelineComponent::showAddStackMenu()
{
    juce::PopupMenu m;
    m.addItem(1, "+ Bassline Layer: [36 ~ 36 48]");
    m.addItem(2, "+ Hi-Hat Groove Layer: [[42 42] 42 [42 42] 46]");
    m.addItem(3, "+ Offbeat Snare Layer: [~ 38 ~ 38]");
    m.addItem(4, "+ Chord Arp Layer: [[60 64 67]]");
    m.addItem(5, "+ Custom Empty Layer: [~ ~ ~ ~]");

    m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        if (result == 1) addStackLayer("36 ~ 36 48");
        else if (result == 2) addStackLayer("[42 42] 42 [42 42] 46");
        else if (result == 3) addStackLayer("~ 38 ~ 38");
        else if (result == 4) addStackLayer("[60 64 67]");
        else if (result == 5) addStackLayer("~ ~ ~ ~");
    });
}

void ArrangementTimelineComponent::addStackLayer(const juce::String& layerPattern)
{
    juce::String cur = tidalPatternEditor.getText().trim();
    if (cur.isEmpty()) cur = "[60 [62 64] 67 [69 71 72]]";

    if (cur.startsWith("[") && cur.endsWith("]"))
    {
        juce::String inner = cur.substring(1, cur.length() - 1).trim();
        tidalPatternEditor.setText("[" + inner + ", " + layerPattern + "]");
    }
    else
    {
        tidalPatternEditor.setText("[" + cur + ", " + layerPattern + "]");
    }
    commitTidalPattern();
}

void ArrangementTimelineComponent::deleteStackLayer(int channelIndex)
{
    juce::String cur = tidalPatternEditor.getText().trim();
    if (!cur.contains(",")) return;

    if (cur.startsWith("[") && cur.endsWith("]"))
    {
        cur = cur.substring(1, cur.length() - 1).trim();
    }

    juce::StringArray layers;
    // Split by comma ignoring inner brackets
    int bracketDepth = 0;
    int lastStart = 0;
    for (int i = 0; i < cur.length(); ++i)
    {
        if (cur[i] == '[') bracketDepth++;
        else if (cur[i] == ']') bracketDepth--;
        else if (cur[i] == ',' && bracketDepth == 0)
        {
            layers.add(cur.substring(lastStart, i).trim());
            lastStart = i + 1;
        }
    }
    if (lastStart < cur.length())
    {
        layers.add(cur.substring(lastStart).trim());
    }

    if (channelIndex >= 0 && channelIndex < layers.size() && layers.size() > 1)
    {
        layers.remove(channelIndex);
        juce::String newPat = "[" + layers.joinIntoString(", ") + "]";
        tidalPatternEditor.setText(newPat);
        commitTidalPattern();
    }
}

void ArrangementTimelineComponent::startInlineBlockEditing(TimelineClip& clip, int eventIdx, const juce::Rectangle<float>& blockBounds)
{
    if (eventIdx < 0 || eventIdx >= static_cast<int>(clip.cachedEvents.size())) return;
    auto& ev = clip.cachedEvents[static_cast<size_t>(eventIdx)];

    isInlineEditingBlock = true;
    inlineEditingEventIdx = eventIdx;
    selectedTidalEventIdx = eventIdx;
    selectedTidalChannel = ev.channel;

    juce::String initText = ev.isRest ? "~" : (!ev.valueStr.empty() ? juce::String(ev.valueStr) : juce::String(ev.pitch));
    inlineBlockEditor.setText(initText);
    inlineBlockEditor.setBounds(blockBounds.toNearestInt().expanded(4, 4));
    inlineBlockEditor.setVisible(true);
    inlineBlockEditor.toFront(true);
    inlineBlockEditor.grabKeyboardFocus();
    inlineBlockEditor.selectAll();
}

void ArrangementTimelineComponent::commitInlineBlockEditing()
{
    if (!isInlineEditingBlock) return;
    isInlineEditingBlock = false;
    inlineBlockEditor.setVisible(false);

    juce::String newText = inlineBlockEditor.getText().trim();
    if (newText.isEmpty()) return;

    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            if (inlineEditingEventIdx >= 0 && inlineEditingEventIdx < static_cast<int>(clip.cachedEvents.size()))
            {
                auto& ev = clip.cachedEvents[static_cast<size_t>(inlineEditingEventIdx)];
                ev.valueStr = newText.toStdString();
                ev.isRest = (newText == "~");
                if (!ev.isRest && newText.containsOnly("0123456789"))
                {
                    ev.pitch = newText.getIntValue();
                }

                // Rebuild pattern
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& e = clip.cachedEvents[i];
                    if (lastChan >= 0 && e.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = e.channel;
                    if (e.isRest) ss << "~";
                    else if (!e.valueStr.empty()) ss << e.valueStr;
                    else ss << e.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                break;
            }
        }
    }
}

ArrangementTimelineComponent::~ArrangementTimelineComponent()
{
    stopTimer();
}

void ArrangementTimelineComponent::setTrackArmed(int trackIdx, bool armed)
{
    while (static_cast<int>(tracks.size()) <= trackIdx)
    {
        TimelineTrackInfo info;
        info.trackIndex = static_cast<int>(tracks.size());
        info.name = "Track " + juce::String(info.trackIndex + 1);
        tracks.push_back(info);
    }
    tracks[static_cast<size_t>(trackIdx)].isArmed = armed;
    repaint();
}

bool ArrangementTimelineComponent::isTrackArmed(int trackIdx) const
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        return tracks[static_cast<size_t>(trackIdx)].isArmed;
    }
    return false;
}

void ArrangementTimelineComponent::setTrackInputSource(int trackIdx, const AudioInputSource& source)
{
    while (static_cast<int>(tracks.size()) <= trackIdx)
    {
        TimelineTrackInfo info;
        info.trackIndex = static_cast<int>(tracks.size());
        info.name = "Track " + juce::String(info.trackIndex + 1);
        tracks.push_back(info);
    }
    tracks[static_cast<size_t>(trackIdx)].inputSource = source;
    repaint();
}

AudioInputSource ArrangementTimelineComponent::getTrackInputSource(int trackIdx) const
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        return tracks[static_cast<size_t>(trackIdx)].inputSource;
    }
    return AudioInputSource();
}

void ArrangementTimelineComponent::showTrackInputMenu(int trackIdx)
{
    juce::PopupMenu m;
    m.addSectionHeader("External Hardware Inputs");
    m.addItem(1, "Input 1 (Mono - Mic/Line 1)");
    m.addItem(2, "Input 2 (Mono - Mic/Line 2)");
    m.addItem(3, "Input 1 + 2 (Stereo)");

    m.addSeparator();
    m.addSectionHeader("Internal App Sources");
    m.addItem(10, "Master Mix (Internal)");

    juce::PopupMenu tapMenu;
    const auto& nodes = nodeGraph.getNodes();
    int tapIdBase = 100;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const auto& n = nodes[i];
        bool hasAudioOutlet = false;
        for (const auto& o : n->getOutlets())
        {
            if (o.dataType == PortDataType::Audio) { hasAudioOutlet = true; break; }
        }
        if (hasAudioOutlet)
        {
            tapMenu.addItem(tapIdBase + static_cast<int>(i), "Node " + std::to_string(n->getId()) + ": " + n->getLabel());
        }
    }
    m.addSubMenu("Tap from Node", tapMenu);

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, trackIdx, nodes, tapIdBase](int res) {
        if (res == 1)
        {
            AudioInputSource src;
            src.type = AudioInputType::ExternalMono;
            src.primaryChannel = 1;
            setTrackInputSource(trackIdx, src);
        }
        else if (res == 2)
        {
            AudioInputSource src;
            src.type = AudioInputType::ExternalMono;
            src.primaryChannel = 2;
            setTrackInputSource(trackIdx, src);
        }
        else if (res == 3)
        {
            AudioInputSource src;
            src.type = AudioInputType::ExternalStereo;
            src.primaryChannel = 1;
            src.secondaryChannel = 2;
            setTrackInputSource(trackIdx, src);
        }
        else if (res == 10)
        {
            AudioInputSource src;
            src.type = AudioInputType::InternalMaster;
            setTrackInputSource(trackIdx, src);
        }
        else if (res >= tapIdBase && res < tapIdBase + static_cast<int>(nodes.size()))
        {
            int nIdx = res - tapIdBase;
            if (nIdx >= 0 && nIdx < static_cast<int>(nodes.size()))
            {
                AudioInputSource src;
                src.type = AudioInputType::InternalNodeTap;
                src.tapNodeId = nodes[static_cast<size_t>(nIdx)]->getId();
                src.tapOutletIndex = 0;
                setTrackInputSource(trackIdx, src);
            }
        }
    });
}

int ArrangementTimelineComponent::getTrackTopY(int trackIdx) const
{
    int y = transportBarHeight + rulerHeight;
    for (int t = 0; t < trackIdx && t < static_cast<int>(tracks.size()); ++t)
    {
        y += tracks[static_cast<size_t>(t)].customHeight;
    }
    return y;
}

int ArrangementTimelineComponent::getTrackHeight(int trackIdx) const
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        return tracks[static_cast<size_t>(trackIdx)].customHeight;
    }
    return trackHeight;
}

int ArrangementTimelineComponent::getTrackIndexAtY(float y) const
{
    int curY = transportBarHeight + rulerHeight;
    for (size_t t = 0; t < tracks.size(); ++t)
    {
        int h = tracks[t].customHeight;
        if (y >= curY && y < curY + h) return static_cast<int>(t);
        curY += h;
    }
    return -1;
}

int ArrangementTimelineComponent::getTotalTracksHeight() const
{
    int total = 0;
    for (const auto& trk : tracks) total += trk.customHeight;
    return total;
}

void ArrangementTimelineComponent::addTrack(const juce::String& name, ClipType type)
{
    TimelineTrackInfo info;
    info.trackId = static_cast<int>(tracks.size()) + 1;
    info.trackIndex = static_cast<int>(tracks.size());
    info.name = name.isNotEmpty() ? name : ("Track " + juce::String(info.trackIndex + 1));
    info.defaultClipType = type;
    info.isArmed = false;
    info.isMuted = false;
    info.isSoloed = false;
    info.customHeight = 58;

    static const juce::Colour palette[] = {
        juce::Colour(0xff00d4ff), // Cyan
        juce::Colour(0xffffd700), // Gold
        juce::Colour(0xff8a2be2), // Violet
        juce::Colour(0xffff4081), // Crimson
        juce::Colour(0xff00e676), // Green
        juce::Colour(0xffff9100)  // Orange
    };
    info.trackColour = palette[tracks.size() % 6];
    tracks.push_back(info);
    repaint();
}

void ArrangementTimelineComponent::deleteTrack(int trackIdx)
{
    if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks.size()) || tracks.size() <= 1) return;

    clips.erase(std::remove_if(clips.begin(), clips.end(), [trackIdx](const TimelineClip& c) {
        return c.trackIndex == trackIdx;
    }), clips.end());

    for (auto& c : clips)
    {
        if (c.trackIndex > trackIdx) c.trackIndex--;
    }

    messageEvents.erase(std::remove_if(messageEvents.begin(), messageEvents.end(), [trackIdx](const TimelineMessageEvent& ev) {
        return ev.trackIndex == trackIdx;
    }), messageEvents.end());

    for (auto& ev : messageEvents)
    {
        if (ev.trackIndex > trackIdx) ev.trackIndex--;
    }

    tracks.erase(tracks.begin() + trackIdx);
    for (size_t t = 0; t < tracks.size(); ++t)
    {
        tracks[t].trackIndex = static_cast<int>(t);
    }
    repaint();
}

void ArrangementTimelineComponent::duplicateTrack(int trackIdx)
{
    if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks.size())) return;

    TimelineTrackInfo orig = tracks[static_cast<size_t>(trackIdx)];
    TimelineTrackInfo dup = orig;
    int newIdx = trackIdx + 1;
    dup.trackId = static_cast<int>(tracks.size()) + 1;
    dup.trackIndex = newIdx;
    dup.name = orig.name + " (Copy)";

    for (auto& c : clips)
    {
        if (c.trackIndex >= newIdx) c.trackIndex++;
    }
    for (auto& ev : messageEvents)
    {
        if (ev.trackIndex >= newIdx) ev.trackIndex++;
    }

    tracks.insert(tracks.begin() + newIdx, dup);
    for (size_t t = 0; t < tracks.size(); ++t)
    {
        tracks[t].trackIndex = static_cast<int>(t);
    }

    std::vector<TimelineClip> newClips;
    static int nextDupClipId = 5000;
    for (const auto& c : clips)
    {
        if (c.trackIndex == trackIdx)
        {
            TimelineClip copyClip = c;
            copyClip.clipId = nextDupClipId++;
            copyClip.trackIndex = newIdx;
            newClips.push_back(copyClip);
        }
    }
    clips.insert(clips.end(), newClips.begin(), newClips.end());
    repaint();
}

void ArrangementTimelineComponent::moveTrack(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= static_cast<int>(tracks.size()) ||
        toIdx < 0 || toIdx >= static_cast<int>(tracks.size()) ||
        fromIdx == toIdx) return;

    auto trk = tracks[static_cast<size_t>(fromIdx)];
    tracks.erase(tracks.begin() + fromIdx);
    tracks.insert(tracks.begin() + toIdx, trk);

    for (size_t t = 0; t < tracks.size(); ++t)
    {
        tracks[t].trackIndex = static_cast<int>(t);
    }

    for (auto& c : clips)
    {
        if (c.trackIndex == fromIdx)
        {
            c.trackIndex = toIdx;
        }
        else if (fromIdx < toIdx && c.trackIndex > fromIdx && c.trackIndex <= toIdx)
        {
            c.trackIndex--;
        }
        else if (fromIdx > toIdx && c.trackIndex >= toIdx && c.trackIndex < fromIdx)
        {
            c.trackIndex++;
        }
    }

    for (auto& ev : messageEvents)
    {
        if (ev.trackIndex == fromIdx)
        {
            ev.trackIndex = toIdx;
        }
        else if (fromIdx < toIdx && ev.trackIndex > fromIdx && ev.trackIndex <= toIdx)
        {
            ev.trackIndex--;
        }
        else if (fromIdx > toIdx && ev.trackIndex >= toIdx && ev.trackIndex < fromIdx)
        {
            ev.trackIndex++;
        }
    }

    repaint();
}

void ArrangementTimelineComponent::setTrackMute(int trackIdx, bool muted)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].isMuted = muted;
        repaint();
    }
}

void ArrangementTimelineComponent::setTrackSolo(int trackIdx, bool soloed)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].isSoloed = soloed;
        repaint();
    }
}

void ArrangementTimelineComponent::toggleTrackMute(int trackIdx)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        setTrackMute(trackIdx, !tracks[static_cast<size_t>(trackIdx)].isMuted);
    }
}

void ArrangementTimelineComponent::toggleTrackSolo(int trackIdx)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        setTrackSolo(trackIdx, !tracks[static_cast<size_t>(trackIdx)].isSoloed);
    }
}

void ArrangementTimelineComponent::setTrackVolume(int trackIdx, float vol)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].volume = std::clamp(vol, 0.0f, 2.0f);
        repaint();
    }
}

void ArrangementTimelineComponent::setTrackPan(int trackIdx, float pan)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].pan = std::clamp(pan, -1.0f, 1.0f);
        repaint();
    }
}

void ArrangementTimelineComponent::setTrackHeight(int trackIdx, int heightPx)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].customHeight = std::clamp(heightPx, 32, 220);
        repaint();
    }
}

void ArrangementTimelineComponent::setTrackColour(int trackIdx, juce::Colour col)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].trackColour = col;
        repaint();
    }
}

void ArrangementTimelineComponent::renameTrack(int trackIdx, const juce::String& newName)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()) && newName.isNotEmpty())
    {
        tracks[static_cast<size_t>(trackIdx)].name = newName;
        repaint();
    }
}

void ArrangementTimelineComponent::setTrackTargetNode(int trackIdx, int nodeId)
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        tracks[static_cast<size_t>(trackIdx)].targetNodeId = nodeId;
        repaint();
    }
}

int ArrangementTimelineComponent::getTrackTargetNode(int trackIdx) const
{
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
    {
        return tracks[static_cast<size_t>(trackIdx)].targetNodeId;
    }
    return -1;
}

void ArrangementTimelineComponent::clearTrackClips(int trackIdx)
{
    clips.erase(std::remove_if(clips.begin(), clips.end(), [trackIdx](const TimelineClip& c) {
        return c.trackIndex == trackIdx;
    }), clips.end());
    repaint();
}

double ArrangementTimelineComponent::quantizeTime(double timeSec, RecordingQuantizeMode mode, double bpm)
{
    if (mode == RecordingQuantizeMode::None) return std::max(0.0, timeSec);
    double beatSec = 60.0 / std::max(1.0, bpm);
    double intervalSec = beatSec;
    if (mode == RecordingQuantizeMode::Sixteenth) intervalSec = beatSec * 0.25;
    else if (mode == RecordingQuantizeMode::Eighth) intervalSec = beatSec * 0.5;
    else if (mode == RecordingQuantizeMode::Beat) intervalSec = beatSec;
    else if (mode == RecordingQuantizeMode::HalfBar) intervalSec = beatSec * 2.0;
    else if (mode == RecordingQuantizeMode::Bar) intervalSec = beatSec * 4.0;
    return std::max(0.0, std::round(timeSec / intervalSec) * intervalSec);
}

double ArrangementTimelineComponent::quantizeDuration(double durationSec, RecordingQuantizeMode mode, double bpm)
{
    if (mode == RecordingQuantizeMode::None) return std::max(0.25, durationSec);
    double beatSec = 60.0 / std::max(1.0, bpm);
    double intervalSec = beatSec;
    if (mode == RecordingQuantizeMode::Sixteenth) intervalSec = beatSec * 0.25;
    else if (mode == RecordingQuantizeMode::Eighth) intervalSec = beatSec * 0.5;
    else if (mode == RecordingQuantizeMode::Beat) intervalSec = beatSec;
    else if (mode == RecordingQuantizeMode::HalfBar) intervalSec = beatSec * 2.0;
    else if (mode == RecordingQuantizeMode::Bar) intervalSec = beatSec * 4.0;
    return std::max(intervalSec, std::round(durationSec / intervalSec) * intervalSec);
}

juce::String ArrangementTimelineComponent::getQuantizeModeName(RecordingQuantizeMode mode)
{
    switch (mode)
    {
        case RecordingQuantizeMode::None: return "OFF";
        case RecordingQuantizeMode::Sixteenth: return "1/16 NOTE";
        case RecordingQuantizeMode::Eighth: return "1/8 NOTE";
        case RecordingQuantizeMode::Beat: return "1 BEAT";
        case RecordingQuantizeMode::HalfBar: return "1/2 BAR";
        case RecordingQuantizeMode::Bar: return "1 BAR";
    }
    return "1 BAR";
}

void ArrangementTimelineComponent::setRecordingQuantizeMode(RecordingQuantizeMode mode)
{
    globalRecQuantize = mode;
    recQuantizeButton.setButtonText("⏱ Q: " + getQuantizeModeName(mode));
    repaint();
}

void ArrangementTimelineComponent::showRecordingQuantizeMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader("Recording Quantization Grid");
    m.addItem(1, "Off (Unquantized / Free Time)", true, globalRecQuantize == RecordingQuantizeMode::None);
    m.addItem(2, "1/16 Note Grid", true, globalRecQuantize == RecordingQuantizeMode::Sixteenth);
    m.addItem(3, "1/8 Note Grid", true, globalRecQuantize == RecordingQuantizeMode::Eighth);
    m.addItem(4, "1 Beat (1/4 Note Grid)", true, globalRecQuantize == RecordingQuantizeMode::Beat);
    m.addItem(5, "1/2 Bar Grid (2 Beats)", true, globalRecQuantize == RecordingQuantizeMode::HalfBar);
    m.addItem(6, "1 Bar Grid (4 Beats / Loop Quantize)", true, globalRecQuantize == RecordingQuantizeMode::Bar);

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&recQuantizeButton), [this](int res) {
        if (res == 1) setRecordingQuantizeMode(RecordingQuantizeMode::None);
        else if (res == 2) setRecordingQuantizeMode(RecordingQuantizeMode::Sixteenth);
        else if (res == 3) setRecordingQuantizeMode(RecordingQuantizeMode::Eighth);
        else if (res == 4) setRecordingQuantizeMode(RecordingQuantizeMode::Beat);
        else if (res == 5) setRecordingQuantizeMode(RecordingQuantizeMode::HalfBar);
        else if (res == 6) setRecordingQuantizeMode(RecordingQuantizeMode::Bar);
    });
}

void ArrangementTimelineComponent::togglePlayback()
{
    if (isTimelinePlaying)
    {
        // Stopping playback: commit any live recordings
        if (wasPlayingPreviousPass && !trackRecordingBuffers.empty())
        {
            double rawStart = recordingStartPlayheadTime;
            double rawDur = std::max(0.1, playheadTimeSec - recordingStartPlayheadTime);
            static int recClipCounter = 1;

            for (const auto& [trackIdx, samples] : trackRecordingBuffers)
            {
                if (samples.size() >= 256)
                {
                    RecordingQuantizeMode qMode = globalRecQuantize;
                    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()) && tracks[static_cast<size_t>(trackIdx)].useTrackQuantizeOverride)
                    {
                        qMode = tracks[static_cast<size_t>(trackIdx)].quantizeOverride;
                    }

                    double quantizedStart = quantizeTime(rawStart, qMode, bpm);
                    double quantizedDur = quantizeDuration(rawDur, qMode, bpm);

                    int targetSampleCount = (qMode == RecordingQuantizeMode::None) ? static_cast<int>(samples.size()) : static_cast<int>(quantizedDur * 96000.0);
                    targetSampleCount = std::max(512, targetSampleCount);

                    std::string tblName = "rec_t" + std::to_string(trackIdx + 1) + "_" + std::to_string(recClipCounter);
                    juce::AudioBuffer<float> finalBuf(1, targetSampleCount);
                    finalBuf.clear();

                    int copyLen = std::min(targetSampleCount, static_cast<int>(samples.size()));
                    finalBuf.copyFrom(0, 0, samples.data(), copyLen);

                    // Smooth edge fades (3ms) to prevent audio clicks
                    int fadeSamples = std::min(288, targetSampleCount / 8);
                    if (fadeSamples > 0)
                    {
                        finalBuf.applyGainRamp(0, 0, fadeSamples, 0.0f, 1.0f);
                        finalBuf.applyGainRamp(0, copyLen - fadeSamples, fadeSamples, 1.0f, 0.0f);
                    }

                    TableManager::getInstance().registerBuffer(tblName, finalBuf, 96000.0);

                    TimelineClip c;
                    c.clipId = 1000 + recClipCounter++;
                    c.trackIndex = trackIdx;
                    c.startTimeSec = quantizedStart;
                    c.durationSec = quantizedDur;
                    c.name = "Audio Rec " + juce::String(trackIdx + 1) + " [" + getQuantizeModeName(qMode) + "]";
                    c.type = ClipType::AudioSample;
                    c.sampleTableName = tblName;
                    c.color = juce::Colour(0xffd02040);
                    clips.push_back(c);
                }
            }
            trackRecordingBuffers.clear();
            wasPlayingPreviousPass = false;
        }
    }

    isTimelinePlaying = !isTimelinePlaying;
    playStopButton.setButtonText(isTimelinePlaying ? "STOP" : "PLAY");
    playStopButton.setColour(juce::TextButton::textColourOffId, isTimelinePlaying ? juce::Colours::deeppink : CarbonGoldLookAndFeel::goldAccent);
    if (onPlaybackToggled) onPlaybackToggled(isTimelinePlaying);
    repaint();
}

void ArrangementTimelineComponent::rewindToStart()
{
    playheadTimeSec = isLoopEnabled ? loopStartSec : 0.0;
    for (auto& ev : messageEvents) ev.triggeredInCurrentPass = false;
    repaint();
}

void ArrangementTimelineComponent::addClip(int trackIdx, double startSec, double durationSec, const juce::String& name, ClipType type)
{
    static int nextClipId = 1;
    TimelineClip clip;
    clip.clipId = nextClipId++;
    clip.trackIndex = trackIdx;
    clip.startTimeSec = std::max(0.0, startSec);
    clip.durationSec = std::max(0.5, durationSec);
    clip.name = name;
    clip.type = type;

    if (type == ClipType::Automation)
    {
        clip.color = CarbonGoldLookAndFeel::royalViolet;
    }
    else if (type == ClipType::AudioSample)
    {
        clip.color = juce::Colour(0xffd02040);
    }
    else
    {
        clip.color = CarbonGoldLookAndFeel::cyberCyan;
    }

    if (type == ClipType::Pattern)
    {
        clip.updateTidalPattern(clip.tidalPattern);
    }

    clips.push_back(clip);
    selectedClipId = clip.clipId;
    repaint();
}

void ArrangementTimelineComponent::setClipTidalPattern(int clipId, const std::string& pat)
{
    for (auto& clip : clips)
    {
        if (clip.clipId == clipId)
        {
            clip.updateTidalPattern(pat);
            selectedClipId = clip.clipId;
            tidalPatternEditor.setText(pat);
            repaint();
            break;
        }
    }
}

void ArrangementTimelineComponent::applySubdivisionMacro(int division)
{
    juce::String cur = tidalPatternEditor.getText().trim();
    if (cur.isEmpty())
    {
        for (const auto& c : clips)
        {
            if (c.clipId == selectedClipId) { cur = c.tidalPattern; break; }
        }
    }
    if (cur.isEmpty()) cur = "[60 [62 64] 67 [69 71 72]]";

    if (division == 2)
    {
        tidalPatternEditor.setText("[" + cur + ", [36 48]]");
    }
    else if (division == 3)
    {
        tidalPatternEditor.setText("[" + cur + " [60 64 67]]");
    }
    commitTidalPattern();
}

void ArrangementTimelineComponent::applyStackMacro()
{
    addStackLayer("36 [~ 48]");
}

void ArrangementTimelineComponent::applyEuclideanMacro(int k, int n)
{
    juce::String cur = tidalPatternEditor.getText().trim();
    tidalPatternEditor.setText("[" + cur + ", 36(" + juce::String(k) + "," + juce::String(n) + ")]");
    commitTidalPattern();
}

void ArrangementTimelineComponent::applyAlternateMacro()
{
    juce::String cur = tidalPatternEditor.getText().trim();
    tidalPatternEditor.setText("<" + cur + " [67 69 71 72]>");
    commitTidalPattern();
}

void ArrangementTimelineComponent::applySpeedMacro(double mult)
{
    juce::String cur = tidalPatternEditor.getText().trim();
    tidalPatternEditor.setText("[" + cur + "]*" + juce::String(mult, 1));
    commitTidalPattern();
}

void ArrangementTimelineComponent::applyDegradeMacro()
{
    juce::String cur = tidalPatternEditor.getText().trim();
    tidalPatternEditor.setText("[" + cur + "]?0.75");
    commitTidalPattern();
    commitTidalPattern();
}

void ArrangementTimelineComponent::commitTidalPattern()
{
    juce::String pat = tidalPatternEditor.getText().trim();
    if (pat.isEmpty()) return;

    std::string patStr = pat.toStdString();

    for (auto& clip : clips)
    {
        if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
        {
            clip.updateTidalPattern(patStr);
            selectedClipId = clip.clipId;
            break;
        }
    }

    // Also dispatch message to any tidal / seq nodes in the graph
    for (auto& node : nodeGraph.getNodes())
    {
        std::string sym = node->getSymbol();
        if (sym == "seq.tidal" || sym == "tidal" || sym == "pattern")
        {
            node->receiveMessage("pat " + patStr);
        }
    }

    repaint();
}

void ArrangementTimelineComponent::deleteClip(int clipId)
{
    clips.erase(std::remove_if(clips.begin(), clips.end(), [clipId](const TimelineClip& c) {
        return c.clipId == clipId;
    }), clips.end());
    if (selectedClipId == clipId) selectedClipId = -1;
    repaint();
}

void ArrangementTimelineComponent::duplicateSelectedClip()
{
    for (const auto& c : clips)
    {
        if (c.clipId == selectedClipId)
        {
            addClip(c.trackIndex, c.startTimeSec + c.durationSec, c.durationSec, c.name + " (Copy)", c.type);
            break;
        }
    }
}

void ArrangementTimelineComponent::splitClipAtPlayhead()
{
    for (size_t i = 0; i < clips.size(); ++i)
    {
        auto& c = clips[i];
        if (c.clipId == selectedClipId || (playheadTimeSec > c.startTimeSec && playheadTimeSec < c.startTimeSec + c.durationSec))
        {
            double splitTime = playheadTimeSec;
            if (splitTime > c.startTimeSec + 0.1 && splitTime < c.startTimeSec + c.durationSec - 0.1)
            {
                double oldDur = c.durationSec;
                c.durationSec = splitTime - c.startTimeSec;
                addClip(c.trackIndex, splitTime, oldDur - c.durationSec, c.name + " (Pt 2)", c.type);
                break;
            }
        }
    }
}

void ArrangementTimelineComponent::addMessageEvent(int targetNodeId, int trackIdx, double timeSec, const juce::String& msgText)
{
    static int nextEventId = 1;
    TimelineMessageEvent ev;
    ev.eventId = nextEventId++;
    ev.targetNodeId = targetNodeId;
    ev.trackIndex = trackIdx;
    ev.timeSec = timeSec;
    ev.messageText = msgText;
    ev.color = CarbonGoldLookAndFeel::goldAccent;
    ev.triggeredInCurrentPass = false;

    messageEvents.push_back(ev);
    repaint();
}

void ArrangementTimelineComponent::deleteMessageEvent(int eventId)
{
    messageEvents.erase(
        std::remove_if(messageEvents.begin(), messageEvents.end(),
                       [eventId](const TimelineMessageEvent& ev) { return ev.eventId == eventId; }),
        messageEvents.end());
    if (selectedEventId == eventId) selectedEventId = -1;
    repaint();
}

void ArrangementTimelineComponent::spawnEventEditor(int targetNodeId, int trackIdx, double timeSec, int existingEventId)
{
    isEditingEvent = true;
    editingEventId = existingEventId;
    editingTargetNodeId = targetNodeId;
    editingTrackIndex = trackIdx;
    editingTimeSec = timeSec;

    if (existingEventId != -1)
    {
        for (const auto& ev : messageEvents)
        {
            if (ev.eventId == existingEventId)
            {
                eventEditor.setText(ev.messageText);
                break;
            }
        }
    }
    else
    {
        eventEditor.setText("stop 600");
    }

    int timelineW = getWidth() - trackHeaderWidth;
    int x = trackHeaderWidth + static_cast<int>((timeSec / totalDurationSec) * timelineW);
    int y = transportBarHeight + rulerHeight + trackIdx * trackHeight + 15;

    eventEditor.setBounds(x - 40, y, 120, 24);
    eventEditor.setVisible(true);
    eventEditor.grabKeyboardFocus();
    eventEditor.selectAll();
}

void ArrangementTimelineComponent::commitEventEditor()
{
    if (!isEditingEvent) return;
    juce::String text = eventEditor.getText().trim();
    eventEditor.setVisible(false);
    isEditingEvent = false;

    if (text.isNotEmpty())
    {
        if (editingEventId != -1)
        {
            for (auto& ev : messageEvents)
            {
                if (ev.eventId == editingEventId)
                {
                    ev.messageText = text;
                    break;
                }
            }
        }
        else
        {
            addMessageEvent(editingTargetNodeId, editingTrackIndex, editingTimeSec, text);
        }
    }
    repaint();
}

void ArrangementTimelineComponent::refreshTimeline()
{
    clips.clear();
    const auto& nodes = nodeGraph.getNodes();

    int trackIdx = 0;
    for (const auto& node : nodes)
    {
        std::string sym = node->getSymbol();
        std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

        if (sym == "seq" || sym == "seq.euclid" || sym == "seq.arp" || sym == "seq.poly" || sym == "step" || sym == "drum")
        {
            TimelineClip c;
            c.clipId = static_cast<int>(clips.size()) + 1;
            c.trackIndex = trackIdx;
            c.startTimeSec = 0.0;
            c.durationSec = 8.0;
            c.name = node->getLabel();
            c.type = ClipType::Pattern;
            c.color = CarbonGoldLookAndFeel::cyberCyan;
            clips.push_back(c);

            // Add second phrase
            TimelineClip c2;
            c2.clipId = static_cast<int>(clips.size()) + 1;
            c2.trackIndex = trackIdx;
            c2.startTimeSec = 8.0;
            c2.durationSec = 8.0;
            c2.name = node->getLabel() + " (Var)";
            c2.type = ClipType::Pattern;
            c2.color = CarbonGoldLookAndFeel::goldAccent;
            clips.push_back(c2);
        }
        else if (sym == "time.curve~" || sym == "time.chaos~" || sym == "auto~")
        {
            TimelineClip c;
            c.clipId = static_cast<int>(clips.size()) + 1;
            c.trackIndex = trackIdx;
            c.startTimeSec = 0.0;
            c.durationSec = 16.0;
            c.name = node->getLabel() + " Automation";
            c.type = ClipType::Automation;
            c.color = CarbonGoldLookAndFeel::royalViolet;
            clips.push_back(c);
        }

        trackIdx++;
    }

    if (clips.empty())
    {
        addClip(0, 0.0, 8.0, "Synth Pattern 1", ClipType::Pattern);
        addClip(0, 8.0, 8.0, "Synth Pattern 2", ClipType::Pattern);
        addClip(1, 0.0, 16.0, "Time Dilation Curve", ClipType::Automation);
    }

    if (!clips.empty() && selectedClipId == -1)
    {
        selectedClipId = clips.front().clipId;
    }

    repaint();
}

void ArrangementTimelineComponent::setPlayheadPosition(double timeInSeconds)
{
    playheadTimeSec = std::clamp(timeInSeconds, 0.0, totalDurationSec);
    repaint();
}

void ArrangementTimelineComponent::timerCallback()
{
    if (isTimelinePlaying)
    {
        lastPlayheadTimeSec = playheadTimeSec;
        playheadTimeSec += (1.0 / 30.0);

        if (isLoopEnabled && playheadTimeSec >= loopEndSec)
        {
            playheadTimeSec = loopStartSec;
            for (auto& ev : messageEvents) ev.triggeredInCurrentPass = false;
        }
        else if (playheadTimeSec >= totalDurationSec)
        {
            playheadTimeSec = 0.0;
            for (auto& ev : messageEvents) ev.triggeredInCurrentPass = false;
        }

        bool anyTrackSoloed = false;
        for (const auto& trk : tracks) { if (trk.isSoloed) { anyTrackSoloed = true; break; } }

        // Check and dispatch scheduled timeline message events
        for (auto& ev : messageEvents)
        {
            if (!ev.triggeredInCurrentPass && playheadTimeSec >= ev.timeSec && lastPlayheadTimeSec <= ev.timeSec)
            {
                ev.triggeredInCurrentPass = true;
                bool shouldDispatch = true;
                if (ev.trackIndex >= 0 && ev.trackIndex < static_cast<int>(tracks.size()))
                {
                    const auto& trk = tracks[static_cast<size_t>(ev.trackIndex)];
                    if (trk.isMuted) shouldDispatch = false;
                    if (anyTrackSoloed && !trk.isSoloed) shouldDispatch = false;
                }

                if (shouldDispatch)
                {
                    auto node = nodeGraph.getNode(ev.targetNodeId);
                    if (node)
                    {
                        node->receiveMessage(ev.messageText.toStdString());
                    }
                }
            }
        }

        // Handle Live Audio Track Recording
        bool isAnyTrackArmed = false;
        for (int t = 0; t < static_cast<int>(tracks.size()); ++t)
        {
            if (tracks[static_cast<size_t>(t)].isArmed)
            {
                isAnyTrackArmed = true;
                break;
            }
        }

        if (isAnyTrackArmed)
        {
            if (!wasPlayingPreviousPass)
            {
                wasPlayingPreviousPass = true;
                recordingStartPlayheadTime = playheadTimeSec;
                trackRecordingBuffers.clear();
            }

            juce::AudioBuffer<float> tempInBuf(2, 512);
            for (size_t t = 0; t < tracks.size(); ++t)
            {
                if (tracks[t].isArmed)
                {
                    AudioInputRouter::getInstance().fetchAudioBlock(tracks[t].inputSource, nodeGraph, tempInBuf, 512);
                    auto& recBuf = trackRecordingBuffers[static_cast<int>(t)];
                    const float* inPtr = tempInBuf.getReadPointer(0);
                    for (int s = 0; s < 512; ++s) recBuf.push_back(inPtr[s]);
                }
            }
        }

        // Format time display: Bar.Beat.Tick | MM:SS.CC
        double beatDuration = 60.0 / bpm;
        int totalBeats = static_cast<int>(playheadTimeSec / beatDuration);
        int bar = (totalBeats / 4) + 1;
        int beat = (totalBeats % 4) + 1;
        int tick = static_cast<int>((std::fmod(playheadTimeSec, beatDuration) / beatDuration) * 16.0) + 1;

        int mins = static_cast<int>(playheadTimeSec) / 60;
        int secs = static_cast<int>(playheadTimeSec) % 60;
        int cents = static_cast<int>((playheadTimeSec - std::floor(playheadTimeSec)) * 100.0);

        std::ostringstream oss;
        oss << "Bar " << bar << "." << beat << "." << tick << " | "
            << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs << "."
            << std::setfill('0') << std::setw(2) << cents;

        timeDisplayLabel.setText(oss.str(), juce::dontSendNotification);
        repaint();
    }
}

void ArrangementTimelineComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    // 1. Top Transport Header
    g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.2f));
    g.fillRect(0, 0, getWidth(), transportBarHeight);

    // 2. Timeline Ruler Header
    auto rulerRect = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth), static_cast<float>(transportBarHeight),
                                           b.getWidth() - trackHeaderWidth, static_cast<float>(rulerHeight));
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(rulerRect);

    // Draw Bar & Beat Grid Markers
    float timelineW = b.getWidth() - trackHeaderWidth;
    int totalBars = static_cast<int>(totalDurationSec / (4.0 * (60.0 / bpm))) + 1;

    for (int bar = 0; bar <= totalBars; ++bar)
    {
        double barTime = bar * 4.0 * (60.0 / bpm);
        float x = static_cast<float>(trackHeaderWidth) + static_cast<float>((barTime / totalDurationSec) * timelineW);

        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.5f));
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(transportBarHeight), static_cast<float>(transportBarHeight + rulerHeight));

        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.drawText("BAR " + juce::String(bar + 1), static_cast<int>(x + 4), transportBarHeight + 4, 50, 16, juce::Justification::left);

        // Sub-beat ticks
        for (int beat = 1; beat < 4; ++beat)
        {
            double beatTime = barTime + beat * (60.0 / bpm);
            float bx = static_cast<float>(trackHeaderWidth) + static_cast<float>((beatTime / totalDurationSec) * timelineW);
            g.setColour(juce::Colours::dimgrey);
            g.drawVerticalLine(static_cast<int>(bx), static_cast<float>(transportBarHeight + rulerHeight - 8), static_cast<float>(transportBarHeight + rulerHeight));
        }
    }

    // 3. Loop Region Highlight
    if (isLoopEnabled)
    {
        float loopX1 = static_cast<float>(trackHeaderWidth) + static_cast<float>((loopStartSec / totalDurationSec) * timelineW);
        float loopX2 = static_cast<float>(trackHeaderWidth) + static_cast<float>((loopEndSec / totalDurationSec) * timelineW);

        g.setColour(CarbonGoldLookAndFeel::royalViolet.withAlpha(0.2f));
        g.fillRect(loopX1, static_cast<float>(transportBarHeight + rulerHeight), loopX2 - loopX1, b.getHeight() - (isPianoRollVisible ? pianoRollHeight : 0));

        g.setColour(CarbonGoldLookAndFeel::royalViolet);
        g.drawRect(loopX1, static_cast<float>(transportBarHeight), loopX2 - loopX1, static_cast<float>(rulerHeight), 2.0f);
    }

    // 4. Tracks & Clips Area
    int yStart = transportBarHeight + rulerHeight;
    int tracksVisibleH = getHeight() - yStart - (isPianoRollVisible ? pianoRollHeight : 0);

    for (size_t t = 0; t < tracks.size(); ++t)
    {
        const auto& trk = tracks[t];
        int y = getTrackTopY(static_cast<int>(t));
        int h = trk.customHeight;
        if (y + h > yStart + tracksVisibleH) break;

        // Track Header Panel
        auto headerR = juce::Rectangle<float>(0.0f, static_cast<float>(y), static_cast<float>(trackHeaderWidth), static_cast<float>(h));
        g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.1f));
        g.fillRect(headerR);

        // Color Accent Strip (Left 4px)
        g.setColour(trk.trackColour);
        g.fillRect(0.0f, static_cast<float>(y), 4.0f, static_cast<float>(h));

        // Header Outline
        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.2f));
        g.drawRect(headerR, 1.0f);

        // Drag Handle Dots
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::grey);
        g.drawText("⠿", 6, y + 3, 12, 16, juce::Justification::centred);

        // Track Name
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(trk.isSoloed ? juce::Colour(0xff00e676) : (trk.isMuted ? juce::Colours::grey : CarbonGoldLookAndFeel::goldAccent));
        g.drawText(trk.name, 20, y + 3, trackHeaderWidth - 95, 16, juce::Justification::left);

        // [M] Mute Button
        auto muteR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 72), static_cast<float>(y + 3), 16.0f, 16.0f);
        g.setColour(trk.isMuted ? juce::Colour(0xffffb300) : juce::Colour(0xff22252c));
        g.fillRoundedRectangle(muteR, 2.0f);
        g.setColour(trk.isMuted ? juce::Colours::black : juce::Colours::grey);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("M", muteR.toNearestInt(), juce::Justification::centred);

        // [S] Solo Button
        auto soloR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 52), static_cast<float>(y + 3), 16.0f, 16.0f);
        g.setColour(trk.isSoloed ? juce::Colour(0xff00e676) : juce::Colour(0xff22252c));
        g.fillRoundedRectangle(soloR, 2.0f);
        g.setColour(trk.isSoloed ? juce::Colours::black : juce::Colours::grey);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText("S", soloR.toNearestInt(), juce::Justification::centred);

        // [●] ARM Button
        bool isArmed = trk.isArmed;
        auto armR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 32), static_cast<float>(y + 3), 26.0f, 16.0f);
        g.setColour(isArmed ? juce::Colour(0xffd02040) : juce::Colour(0xff22252c));
        g.fillRoundedRectangle(armR, 2.0f);
        g.setColour(isArmed ? juce::Colours::white : juce::Colour(0xffe57373));
        g.drawRoundedRectangle(armR, 2.0f, 1.0f);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.drawText(isArmed ? "REC" : "ARM", armR.toNearestInt(), juce::Justification::centred);

        // Input Source Selector Badge
        if (h >= 44)
        {
            auto inR = juce::Rectangle<float>(8.0f, static_cast<float>(y + 22), static_cast<float>(trackHeaderWidth - 16), 14.0f);
            g.setColour(juce::Colour(0xff151820));
            g.fillRoundedRectangle(inR, 2.0f);
            g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.35f));
            g.drawRoundedRectangle(inR, 2.0f, 1.0f);
            g.setFont(juce::Font(8.5f, juce::Font::plain));
            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.drawText("🎙 " + trk.inputSource.getDisplayName(), inR.reduced(2, 0).toNearestInt(), juce::Justification::left);
        }

        // Bottom Divider Resize Handle (Bottom 4px)
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawHorizontalLine(y + h - 1, 0.0f, static_cast<float>(trackHeaderWidth));

        // Track Lane Background
        auto laneR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth), static_cast<float>(y), timelineW, static_cast<float>(h));
        juce::Colour laneBg = (t % 2 == 0) ? CarbonGoldLookAndFeel::carbonBg.brighter(0.02f) : CarbonGoldLookAndFeel::carbonBg;
        if (trk.isMuted) laneBg = laneBg.darker(0.3f);
        g.setColour(laneBg);
        g.fillRect(laneR);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine(y + h, static_cast<float>(trackHeaderWidth), b.getWidth());
    }

    // 5. Draw Timeline Clips
    for (const auto& clip : clips)
    {
        if (clip.trackIndex < 0 || clip.trackIndex >= static_cast<int>(tracks.size())) continue;
        int y = getTrackTopY(clip.trackIndex);
        int h = getTrackHeight(clip.trackIndex);
        if (y + h > yStart + tracksVisibleH) continue;

        float cx = static_cast<float>(trackHeaderWidth) + static_cast<float>((clip.startTimeSec / totalDurationSec) * timelineW);
        float cw = static_cast<float>((clip.durationSec / totalDurationSec) * timelineW);
        auto clipR = juce::Rectangle<float>(cx, static_cast<float>(y + 4), cw, static_cast<float>(h - 8));

        bool isSelected = (clip.clipId == selectedClipId);

        g.setColour(clip.color.withAlpha(isSelected ? 0.35f : 0.20f));
        g.fillRoundedRectangle(clipR, 4.0f);

        g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : clip.color);
        g.drawRoundedRectangle(clipR, 4.0f, isSelected ? 2.0f : 1.0f);

        // Clip Title
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(clip.name, static_cast<int>(cx + 6), y + 6, static_cast<int>(cw - 12), 16, juce::Justification::left);

        // Miniature note / automation / audio preview
        if (clip.type == ClipType::Pattern)
        {
            drawTidalSubdivisionBlocks(g, clip, clipR);
        }
        else if (clip.type == ClipType::Automation)
        {
            drawAutomationCurves(g, clip, clipR);
        }
        else if (clip.type == ClipType::AudioSample)
        {
            drawAudioWaveformClip(g, clip, clipR);
        }
    }

    // 6. Draw Scheduled Message Events
    for (const auto& ev : messageEvents)
    {
        if (ev.trackIndex < 0 || ev.trackIndex >= static_cast<int>(tracks.size())) continue;
        int y = getTrackTopY(ev.trackIndex);
        int h = getTrackHeight(ev.trackIndex);
        if (y + h > yStart + tracksVisibleH) continue;

        float ex = static_cast<float>(trackHeaderWidth) + static_cast<float>((ev.timeSec / totalDurationSec) * timelineW);
        auto badgeR = juce::Rectangle<float>(ex - 35.0f, static_cast<float>(y + 12), 70.0f, 22.0f);

        bool isSelected = (ev.eventId == selectedEventId);
        g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::royalViolet);
        g.fillRoundedRectangle(badgeR, 3.0f);

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.setColour(CarbonGoldLookAndFeel::carbonBg);
        g.drawText(ev.messageText, badgeR, juce::Justification::centred);
    }

    // 7. Master Playhead Line
    float px = static_cast<float>(trackHeaderWidth) + static_cast<float>((playheadTimeSec / totalDurationSec) * timelineW);
    g.setColour(juce::Colour(0xffff2050));
    g.drawVerticalLine(static_cast<int>(px), static_cast<float>(transportBarHeight), b.getHeight() - (isPianoRollVisible ? pianoRollHeight : 0));

    juce::Path head;
    head.addTriangle(px - 6.0f, static_cast<float>(transportBarHeight), px + 6.0f, static_cast<float>(transportBarHeight), px, static_cast<float>(transportBarHeight + 10));
    g.fillPath(head);

    // 8. Piano Roll & Tidal Pattern Drawer
    if (isPianoRollVisible)
    {
        auto drawerBounds = juce::Rectangle<float>(0.0f, b.getHeight() - pianoRollHeight, b.getWidth(), static_cast<float>(pianoRollHeight));
        drawPianoRollDrawer(g, drawerBounds);
    }
}

void ArrangementTimelineComponent::drawTidalSubdivisionBlocks(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect)
{
    if (clip.cachedEvents.empty())
    {
        int numSteps = static_cast<int>(clip.stepPitches.size());
        float stepW = clipRect.getWidth() / static_cast<float>(numSteps);
        for (int s = 0; s < numSteps; ++s)
        {
            if (s < static_cast<int>(clip.stepGates.size()) && clip.stepGates[static_cast<size_t>(s)])
            {
                float noteNorm = (clip.stepPitches[static_cast<size_t>(s)] - 36) / 48.0f;
                float ny = clipRect.getBottom() - 4.0f - noteNorm * (clipRect.getHeight() - 24.0f);
                g.setColour(clip.color);
                g.fillRect(clipRect.getX() + s * stepW + 1.0f, ny, stepW - 2.0f, 4.0f);
            }
        }
        return;
    }

    int maxChan = 1;
    for (const auto& ev : clip.cachedEvents) maxChan = std::max(maxChan, ev.channel + 1);
    float chanH = (clipRect.getHeight() - 20.0f) / static_cast<float>(maxChan);

    bool isClipPlaying = isTimelinePlaying && (playheadTimeSec >= clip.startTimeSec && playheadTimeSec < clip.startTimeSec + clip.durationSec);
    double clipPhase = 0.0;
    if (isClipPlaying && clip.durationSec > 0.001)
    {
        clipPhase = std::fmod((playheadTimeSec - clip.startTimeSec) / clip.durationSec, 1.0);
        if (clipPhase < 0.0) clipPhase += 1.0;
    }

    for (const auto& ev : clip.cachedEvents)
    {
        float ex = clipRect.getX() + static_cast<float>(ev.startCycle * clipRect.getWidth());
        float ew = std::max(2.0f, static_cast<float>((ev.endCycle - ev.startCycle) * clipRect.getWidth()) - 1.0f);
        float ey = clipRect.getY() + 18.0f + static_cast<float>(ev.channel) * chanH;

        bool isActive = isClipPlaying && (clipPhase >= ev.startCycle && clipPhase < ev.endCycle);

        juce::Colour blkCol = (ev.channel == 0) ? clip.color :
                             (ev.channel == 1) ? CarbonGoldLookAndFeel::goldAccent :
                                                 CarbonGoldLookAndFeel::royalViolet;

        g.setColour(isActive ? blkCol.brighter(0.6f) : blkCol.withAlpha(ev.velocity * 0.85f));
        g.fillRoundedRectangle(ex, ey + 1.0f, ew, chanH - 2.0f, 2.0f);

        // Strudel-style glowing white border for active step
        if (isActive)
        {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(ex, ey + 1.0f, ew, chanH - 2.0f, 2.0f, 1.8f);
        }
    }
}

void ArrangementTimelineComponent::drawAutomationCurves(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect)
{
    if (clip.automationPoints.size() < 2) return;

    juce::Path p;
    for (size_t i = 0; i < clip.automationPoints.size(); ++i)
    {
        float x = clipRect.getX() + static_cast<float>(clip.automationPoints[i].first) * clipRect.getWidth();
        float y = clipRect.getBottom() - 4.0f - clip.automationPoints[i].second * (clipRect.getHeight() - 16.0f);

        if (i == 0) p.startNewSubPath(x, y);
        else p.lineTo(x, y);

        // Breakpoint dot
        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }

    g.setColour(CarbonGoldLookAndFeel::royalViolet);
    g.strokePath(p, juce::PathStrokeType(2.0f));
}

void ArrangementTimelineComponent::drawAudioWaveformClip(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect)
{
    g.setColour(juce::Colour(0xff2d141e));
    g.fillRoundedRectangle(clipRect, 3.0f);

    const auto* info = TableManager::getInstance().getTableInfo(clip.sampleTableName);
    if (info && !info->thumbnailPeaks.empty())
    {
        g.setColour(juce::Colour(0xffff8a80));
        const auto& peaks = info->thumbnailPeaks;
        float midY = clipRect.getCentreY() + 6.0f;
        float halfH = (clipRect.getHeight() - 16.0f) * 0.45f;
        float stepX = clipRect.getWidth() / static_cast<float>(peaks.size());

        for (size_t i = 0; i < peaks.size(); ++i)
        {
            float px = clipRect.getX() + static_cast<float>(i) * stepX;
            float h = std::clamp(peaks[i] * halfH, 1.0f, halfH);
            g.drawLine(px, midY - h, px, midY + h, 1.0f);
        }
    }
    else
    {
        g.setColour(juce::Colour(0xffff8a80).withAlpha(0.6f));
        g.drawHorizontalLine(static_cast<int>(clipRect.getCentreY() + 6.0f), clipRect.getX(), clipRect.getRight());
    }

    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText("🎙 " + clip.name, clipRect.reduced(4, 2).toNearestInt(), juce::Justification::topLeft);
}

void ArrangementTimelineComponent::drawPianoRollDrawer(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
    g.fillRect(bounds);

    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
    g.drawHorizontalLine(static_cast<int>(bounds.getY()), 0.0f, bounds.getWidth());

    // Drawer Header
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(CarbonGoldLookAndFeel::goldAccent);

    juce::String clipName = "No Clip Selected";
    TimelineClip* activeClip = nullptr;
    for (auto& c : clips)
    {
        if (c.clipId == selectedClipId) { activeClip = &c; clipName = c.name; break; }
    }
    if (!activeClip && !clips.empty())
    {
        activeClip = &clips.front();
        clipName = activeClip->name;
    }

    g.drawText("TIDALCYCLES PATTERN & SUBDIVISION EDITOR: " + clipName, 12, static_cast<int>(bounds.getY() + 4), 420, 20, juce::Justification::left);

    if (!activeClip) return;

    // Draw Dynamic Time-Proportional Tidal Blocks
    float gridY = bounds.getY() + 66.0f;
    float gridH = bounds.getHeight() - 74.0f;
    float areaW = bounds.getWidth() - 24.0f;

    if (activeClip->cachedEvents.empty())
    {
        activeClip->updateTidalPattern(activeClip->tidalPattern);
    }

    int maxChan = 1;
    for (const auto& ev : activeClip->cachedEvents) maxChan = std::max(maxChan, ev.channel + 1);
    float chanH = gridH / static_cast<float>(maxChan);

    // Compute cycle phase for Strudel-style active block playback
    double cyclePhase = 0.0;
    bool isClipActive = false;
    if (isTimelinePlaying && activeClip->durationSec > 0.001)
    {
        if (playheadTimeSec >= activeClip->startTimeSec && playheadTimeSec < activeClip->startTimeSec + activeClip->durationSec)
        {
            cyclePhase = std::fmod((playheadTimeSec - activeClip->startTimeSec) / activeClip->durationSec, 1.0);
            if (cyclePhase < 0.0) cyclePhase += 1.0;
            isClipActive = true;
        }
    }

    // Check if any TidalSeqNode in nodeGraph is playing
    if (!isClipActive)
    {
        for (const auto& n : nodeGraph.getNodes())
        {
            if (auto tn = std::dynamic_pointer_cast<TidalSeqNode>(n))
            {
                cyclePhase = tn->getCyclePhase();
                isClipActive = isTimelinePlaying;
                break;
            }
        }
    }

    // Channel lane separators & background
    for (int ch = 0; ch < maxChan; ++ch)
    {
        float cy = gridY + ch * chanH;
        g.setColour(ch == selectedTidalChannel ? CarbonGoldLookAndFeel::carbonBg.brighter(0.04f) : CarbonGoldLookAndFeel::carbonBg.darker(0.2f));
        g.fillRect(12.0f, cy, areaW, chanH - 4.0f);
        g.setColour(ch == selectedTidalChannel ? CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f) : CarbonGoldLookAndFeel::goldAccent.withAlpha(0.15f));
        g.drawRect(12.0f, cy, areaW, chanH - 4.0f, 1.0f);

        // Lane label & delete button
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.setColour(ch == selectedTidalChannel ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::grey);
        g.drawText("VOICE " + juce::String(ch + 1), 16, static_cast<int>(cy + 2), 60, 14, juce::Justification::left);

        if (maxChan > 1)
        {
            auto delRect = juce::Rectangle<float>(12.0f + areaW - 22.0f, cy + 2.0f, 18.0f, 14.0f);
            g.setColour(juce::Colours::red.withAlpha(0.4f));
            g.fillRoundedRectangle(delRect, 2.0f);
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText("x", delRect, juce::Justification::centred);
        }
    }

    // Draw Event Blocks with Strudel-Style Active Border & Selection Highlights
    for (size_t i = 0; i < activeClip->cachedEvents.size(); ++i)
    {
        const auto& ev = activeClip->cachedEvents[i];
        float ex = 12.0f + static_cast<float>(ev.startCycle * areaW);
        float ew = std::max(6.0f, static_cast<float>((ev.endCycle - ev.startCycle) * areaW) - 3.0f);
        float ey = gridY + ev.channel * chanH + 2.0f;
        float eh = chanH - 8.0f;

        auto blockR = juce::Rectangle<float>(ex, ey, ew, eh);

        bool isActive = isClipActive && (cyclePhase >= ev.startCycle && cyclePhase < ev.endCycle);
        bool isSelected = (static_cast<int>(i) == selectedTidalEventIdx);

        juce::Colour blockCol = (ev.channel == 0) ? CarbonGoldLookAndFeel::cyberCyan :
                                (ev.channel == 1) ? CarbonGoldLookAndFeel::goldAccent :
                                                    juce::Colour(0xffff9900);

        if (isActive)
        {
            // Strudel-style active block fill (bright flash)
            g.setColour(blockCol.brighter(0.7f).withAlpha(0.95f));
            g.fillRoundedRectangle(blockR, 4.0f);

            // Strudel-style crisp glowing WHITE BORDER
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(blockR, 4.0f, 2.5f);

            // Inner white glow
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawRoundedRectangle(blockR.reduced(1.0f), 3.0f, 1.0f);

            g.setFont(juce::Font(ew > 40.0f ? 12.0f : 10.0f, juce::Font::bold));
            g.setColour(CarbonGoldLookAndFeel::carbonBg);
        }
        else
        {
            g.setColour(blockCol.withAlpha(ev.velocity * 0.85f));
            g.fillRoundedRectangle(blockR, 4.0f);

            if (isSelected)
            {
                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(blockR, 4.0f, 2.0f);
            }
            else
            {
                g.setColour(CarbonGoldLookAndFeel::carbonBg.brighter(0.1f));
                g.drawRoundedRectangle(blockR, 4.0f, 1.0f);
            }

            g.setFont(juce::Font(ew > 40.0f ? 11.0f : 9.0f, juce::Font::bold));
            g.setColour(CarbonGoldLookAndFeel::carbonBg);
        }

        // Pitch text / drum label / bracket indicator
        juce::String label = ev.valueStr.empty() ? juce::String(ev.pitch) : juce::String(ev.valueStr);
        g.drawText(label, blockR, juce::Justification::centred);
    }

    // Strudel-style smooth playhead sweep cursor across drawer
    if (isClipActive)
    {
        float sweepX = 12.0f + static_cast<float>(cyclePhase * areaW);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawVerticalLine(static_cast<int>(sweepX), gridY, gridY + gridH);
    }
}

void ArrangementTimelineComponent::resized()
{
    int y = 4;
    int btnH = 26;

    playStopButton.setBounds(10, y, 65, btnH);
    rewindButton.setBounds(80, y, 70, btnH);
    loopButton.setBounds(155, y, 75, btnH);
    addClipButton.setBounds(235, y, 80, btnH);
    addTrackButton.setBounds(320, y, 70, btnH);
    recQuantizeButton.setBounds(395, y, 90, btnH);
    togglePianoRollBtn.setBounds(490, y, 100, btnH);

    timeDisplayLabel.setBounds(getWidth() - 240, y, 230, btnH);

    // Layout Tidal Pattern Editor and Quick Macro buttons in the bottom drawer
    if (isPianoRollVisible)
    {
        int drawerY = getHeight() - pianoRollHeight;
        int py = drawerY + 28;
        int edW = std::clamp(getWidth() - 860, 160, 320);
        tidalPatternEditor.setBounds(12, py, edW, 28);
        tidalPatternEditor.setVisible(true);

        int bx = 12 + edW + 6;

        wrapBracketBtn.setBounds(bx, py, 56, 28); wrapBracketBtn.setVisible(true); bx += 60;
        subdivideBtn.setBounds(bx, py, 46, 28); subdivideBtn.setVisible(true); bx += 50;
        tripletBtn.setBounds(bx, py, 38, 28); tripletBtn.setVisible(true); bx += 42;
        quadBtn.setBounds(bx, py, 38, 28); quadBtn.setVisible(true); bx += 42;
        unwrapBtn.setBounds(bx, py, 58, 28); unwrapBtn.setVisible(true); bx += 62;

        stackBtn.setBounds(bx, py, 94, 28); stackBtn.setVisible(true); bx += 98;
        euclidBtn.setBounds(bx, py, 72, 28); euclidBtn.setVisible(true); bx += 76;
        alternateBtn.setBounds(bx, py, 56, 28); alternateBtn.setVisible(true); bx += 60;
        speed2Btn.setBounds(bx, py, 38, 28); speed2Btn.setVisible(true); bx += 42;
        degradeBtn.setBounds(bx, py, 50, 28); degradeBtn.setVisible(true); bx += 54;
        restBtn.setBounds(bx, py, 52, 28); restBtn.setVisible(true); bx += 56;

        pitchUpBtn.setBounds(bx, py, 30, 28); pitchUpBtn.setVisible(true); bx += 32;
        pitchDownBtn.setBounds(bx, py, 30, 28); pitchDownBtn.setVisible(true); bx += 34;
        octUpBtn.setBounds(bx, py, 36, 28); octUpBtn.setVisible(true); bx += 38;
        octDownBtn.setBounds(bx, py, 36, 28); octDownBtn.setVisible(true); bx += 40;

        applyPatternBtn.setBounds(bx, py, 68, 28); applyPatternBtn.setVisible(true); bx += 72;
        tidalHelpBtn.setBounds(bx, py, 58, 28); tidalHelpBtn.setVisible(true);
    }
    else
    {
        tidalPatternEditor.setVisible(false);
        wrapBracketBtn.setVisible(false);
        subdivideBtn.setVisible(false);
        tripletBtn.setVisible(false);
        quadBtn.setVisible(false);
        unwrapBtn.setVisible(false);
        stackBtn.setVisible(false);
        euclidBtn.setVisible(false);
        alternateBtn.setVisible(false);
        speed2Btn.setVisible(false);
        degradeBtn.setVisible(false);
        restBtn.setVisible(false);
        pitchUpBtn.setVisible(false);
        pitchDownBtn.setVisible(false);
        octUpBtn.setVisible(false);
        octDownBtn.setVisible(false);
        applyPatternBtn.setVisible(false);
        tidalHelpBtn.setVisible(false);
    }
}

void ArrangementTimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);

    // 0. Clicked Track Header Panel: Drag, Resize Divider, Mute, Solo, ARM, Input Menu or Context Menu
    if (pos.x < trackHeaderWidth && pos.y >= transportBarHeight + rulerHeight)
    {
        int t = getTrackIndexAtY(pos.y);
        if (t >= 0 && t < static_cast<int>(tracks.size()))
        {
            int ty = getTrackTopY(t);
            int th = getTrackHeight(t);

            // Check bottom 5px divider for track height resizing
            if (pos.y >= ty + th - 5)
            {
                resizingDividerTrackIdx = t;
                dragStartDividerY = pos.y;
                dragStartTrackHeight = th;
                return;
            }

            // Right-click context menu for track management
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu tm;
                tm.addSectionHeader(tracks[static_cast<size_t>(t)].name + " Options");
                tm.addItem(1, "Rename Track...");
                tm.addSeparator();
                tm.addItem(2, "+ Add Pattern Track");
                tm.addItem(3, "+ Add Audio Sample Track");
                tm.addItem(4, "+ Add Automation Track");
                tm.addItem(5, "Duplicate Track");
                tm.addItem(6, "Delete Track", tracks.size() > 1);
                tm.addSeparator();

                juce::PopupMenu colMenu;
                colMenu.addItem(10, "Cyan (Default)");
                colMenu.addItem(11, "Gold / Amber");
                colMenu.addItem(12, "Royal Violet");
                colMenu.addItem(13, "Crimson Red");
                colMenu.addItem(14, "Neon Green");
                colMenu.addItem(15, "Orange");
                tm.addSubMenu("Track Color", colMenu);

                juce::PopupMenu hMenu;
                hMenu.addItem(20, "Compact (36px)");
                hMenu.addItem(21, "Normal (58px)");
                hMenu.addItem(22, "Tall (95px)");
                hMenu.addItem(23, "Expanded (140px)");
                tm.addSubMenu("Track Height", hMenu);

                tm.addSeparator();
                tm.addItem(30, "Select Audio Input Source...");
                tm.addItem(31, "Clear All Clips on Track");

                tm.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, t](int res) {
                    if (res == 1)
                    {
                        renamingTrackIdx = t;
                        trackRenameEditor.setText(tracks[static_cast<size_t>(t)].name);
                        trackRenameEditor.setBounds(20, getTrackTopY(t) + 2, trackHeaderWidth - 95, 20);
                        trackRenameEditor.setVisible(true);
                        trackRenameEditor.grabKeyboardFocus();
                        trackRenameEditor.selectAll();
                    }
                    else if (res == 2) addTrack("Pattern " + juce::String(tracks.size() + 1), ClipType::Pattern);
                    else if (res == 3) addTrack("Audio " + juce::String(tracks.size() + 1), ClipType::AudioSample);
                    else if (res == 4) addTrack("Auto " + juce::String(tracks.size() + 1), ClipType::Automation);
                    else if (res == 5) duplicateTrack(t);
                    else if (res == 6) deleteTrack(t);
                    else if (res == 10) setTrackColour(t, juce::Colour(0xff00d4ff));
                    else if (res == 11) setTrackColour(t, juce::Colour(0xffffd700));
                    else if (res == 12) setTrackColour(t, juce::Colour(0xff8a2be2));
                    else if (res == 13) setTrackColour(t, juce::Colour(0xffff4081));
                    else if (res == 14) setTrackColour(t, juce::Colour(0xff00e676));
                    else if (res == 15) setTrackColour(t, juce::Colour(0xffff9100));
                    else if (res == 20) setTrackHeight(t, 36);
                    else if (res == 21) setTrackHeight(t, 58);
                    else if (res == 22) setTrackHeight(t, 95);
                    else if (res == 23) setTrackHeight(t, 140);
                    else if (res == 30) showTrackInputMenu(t);
                    else if (res == 31) clearTrackClips(t);
                });
                return;
            }

            // Check [M] Mute Button
            auto muteR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 72), static_cast<float>(ty + 3), 16.0f, 16.0f);
            if (muteR.contains(pos))
            {
                toggleTrackMute(t);
                return;
            }

            // Check [S] Solo Button
            auto soloR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 52), static_cast<float>(ty + 3), 16.0f, 16.0f);
            if (soloR.contains(pos))
            {
                toggleTrackSolo(t);
                return;
            }

            // Check [●] ARM Button
            auto armR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth - 32), static_cast<float>(ty + 3), 26.0f, 16.0f);
            if (armR.contains(pos))
            {
                setTrackArmed(t, !isTrackArmed(t));
                return;
            }

            // Check Input Selector Badge
            if (th >= 44)
            {
                auto inR = juce::Rectangle<float>(8.0f, static_cast<float>(ty + 22), static_cast<float>(trackHeaderWidth - 16), 14.0f);
                if (inR.contains(pos))
                {
                    showTrackInputMenu(t);
                    return;
                }
            }

            // Left drag grip: Start track reordering
            if (pos.x <= 20.0f)
            {
                draggingTrackIdx = t;
                return;
            }
        }
    }

    // 1. Clicked Transport Ruler: Scrub Playhead or Set Loop Range
    if (pos.y >= transportBarHeight && pos.y <= transportBarHeight + rulerHeight)
    {
        double clickTime = ((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec;
        clickTime = std::clamp(clickTime, 0.0, totalDurationSec);

        if (e.mods.isShiftDown())
        {
            isSettingLoop = true;
            loopStartSec = clickTime;
            loopEndSec = clickTime + 4.0;
        }
        else
        {
            isDraggingPlayhead = true;
            setPlayheadPosition(clickTime);
        }
        repaint();
        return;
    }

    // 2. Clicked in Tidal Pattern Drawer: Select or mutate block pitches
    if (isPianoRollVisible && pos.y >= getHeight() - pianoRollHeight)
    {
        float gridY = getHeight() - pianoRollHeight + 66.0f;
        float gridH = pianoRollHeight - 74.0f;
        float areaW = getWidth() - 24.0f;

        for (auto& clip : clips)
        {
            if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
            {
                selectedClipId = clip.clipId;
                tidalPatternEditor.setText(clip.tidalPattern);

                int maxChan = 1;
                for (const auto& ev : clip.cachedEvents) maxChan = std::max(maxChan, ev.channel + 1);
                float chanH = gridH / static_cast<float>(maxChan);

                // Check lane delete buttons
                if (maxChan > 1)
                {
                    for (int ch = 0; ch < maxChan; ++ch)
                    {
                        float cy = gridY + ch * chanH;
                        auto delRect = juce::Rectangle<float>(12.0f + areaW - 22.0f, cy + 2.0f, 18.0f, 14.0f);
                        if (delRect.contains(pos))
                        {
                            deleteStackLayer(ch);
                            return;
                        }
                    }
                }

                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    auto& ev = clip.cachedEvents[i];
                    float ex = 12.0f + static_cast<float>(ev.startCycle * areaW);
                    float ew = std::max(6.0f, static_cast<float>((ev.endCycle - ev.startCycle) * areaW) - 3.0f);
                    float ey = gridY + ev.channel * chanH + 2.0f;
                    float eh = chanH - 8.0f;

                    auto blockR = juce::Rectangle<float>(ex, ey, ew, eh);
                    if (blockR.contains(pos))
                    {
                        selectedTidalEventIdx = static_cast<int>(i);
                        selectedTidalChannel = ev.channel;

                        if (e.mods.isPopupMenu())
                        {
                            showTidalBlockContextMenu(clip, static_cast<int>(i));
                            return;
                        }

                        // Left click starts vertical pitch dragging
                        draggingTidalEventIdx = static_cast<int>(i);
                        tidalDragStartY = pos.y;
                        tidalDragStartPitch = ev.pitch;
                        repaint();
                        return;
                    }
                }
                break;
            }
        }
        return;
    }

    // 3. Clicked in Tracks Area: Select / Drag Clips or Messages
    int yStart = transportBarHeight + rulerHeight;
    int tracksVisibleH = getHeight() - yStart - (isPianoRollVisible ? pianoRollHeight : 0);

    for (const auto& clip : clips)
    {
        if (clip.trackIndex < 0 || clip.trackIndex >= static_cast<int>(tracks.size())) continue;
        int y = getTrackTopY(clip.trackIndex);
        int h = getTrackHeight(clip.trackIndex);
        if (y + h > yStart + tracksVisibleH) continue;

        float cx = static_cast<float>(trackHeaderWidth) + static_cast<float>((clip.startTimeSec / totalDurationSec) * timelineW);
        float cw = static_cast<float>((clip.durationSec / totalDurationSec) * timelineW);
        auto clipR = juce::Rectangle<float>(cx, static_cast<float>(y + 4), cw, static_cast<float>(h - 8));

        if (clipR.contains(pos))
        {
            selectedClipId = clip.clipId;
            draggingClipId = clip.clipId;
            dragStartClipTime = clip.startTimeSec;
            isResizingClipEnd = (pos.x >= clipR.getRight() - 10.0f);
            repaint();
            return;
        }
    }
}

void ArrangementTimelineComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.position;

    // 0. Double-click track name in header to trigger inline rename editor
    if (pos.x < trackHeaderWidth && pos.y >= transportBarHeight + rulerHeight)
    {
        int t = getTrackIndexAtY(pos.y);
        if (t >= 0 && t < static_cast<int>(tracks.size()))
        {
            renamingTrackIdx = t;
            trackRenameEditor.setText(tracks[static_cast<size_t>(t)].name);
            trackRenameEditor.setBounds(20, getTrackTopY(t) + 2, trackHeaderWidth - 95, 20);
            trackRenameEditor.setVisible(true);
            trackRenameEditor.grabKeyboardFocus();
            trackRenameEditor.selectAll();
            return;
        }
    }

    // Double click in Tidal drawer opens in-place text editor directly over the block
    if (isPianoRollVisible && pos.y >= getHeight() - pianoRollHeight)
    {
        float gridY = getHeight() - pianoRollHeight + 66.0f;
        float gridH = pianoRollHeight - 74.0f;
        float areaW = getWidth() - 24.0f;

        for (auto& clip : clips)
        {
            if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
            {
                int maxChan = 1;
                for (const auto& ev : clip.cachedEvents) maxChan = std::max(maxChan, ev.channel + 1);
                float chanH = gridH / static_cast<float>(maxChan);

                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    auto& ev = clip.cachedEvents[i];
                    float ex = 12.0f + static_cast<float>(ev.startCycle * areaW);
                    float ew = std::max(6.0f, static_cast<float>((ev.endCycle - ev.startCycle) * areaW) - 3.0f);
                    float ey = gridY + ev.channel * chanH + 2.0f;
                    float eh = chanH - 8.0f;

                    auto blockR = juce::Rectangle<float>(ex, ey, ew, eh);
                    if (blockR.contains(pos))
                    {
                        startInlineBlockEditing(clip, static_cast<int>(i), blockR);
                        return;
                    }
                }
                break;
            }
        }
        return;
    }

    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);
    if (pos.x > trackHeaderWidth && pos.y > transportBarHeight + rulerHeight)
    {
        int trackIdx = getTrackIndexAtY(pos.y);
        if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()))
        {
            double clickTime = ((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec;
            addClip(trackIdx, clickTime, 4.0, "Sequence Clip " + juce::String(clips.size() + 1), tracks[static_cast<size_t>(trackIdx)].defaultClipType);
        }
    }
}

void ArrangementTimelineComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);

    // 0. Resizing Track Divider
    if (resizingDividerTrackIdx >= 0)
    {
        int deltaY = static_cast<int>(pos.y - dragStartDividerY);
        setTrackHeight(resizingDividerTrackIdx, dragStartTrackHeight + deltaY);
        return;
    }

    // 1. Dragging Track Header (Reordering)
    if (draggingTrackIdx >= 0)
    {
        int targetIdx = getTrackIndexAtY(pos.y);
        if (targetIdx >= 0 && targetIdx < static_cast<int>(tracks.size()) && targetIdx != draggingTrackIdx)
        {
            moveTrack(draggingTrackIdx, targetIdx);
            draggingTrackIdx = targetIdx;
        }
        return;
    }

    // 2. Dragging Tidal Block Pitch Vertically
    if (draggingTidalEventIdx >= 0)
    {
        for (auto& clip : clips)
        {
            if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
            {
                if (draggingTidalEventIdx < static_cast<int>(clip.cachedEvents.size()))
                {
                    float deltaY = tidalDragStartY - pos.y;
                    int pitchDelta = static_cast<int>(deltaY / 6.0f); // 6 pixels per semitone
                    auto& ev = clip.cachedEvents[static_cast<size_t>(draggingTidalEventIdx)];
                    ev.pitch = std::clamp(tidalDragStartPitch + pitchDelta, 24, 96);
                    ev.valueStr = juce::String(ev.pitch).toStdString();
                    ev.isRest = false;
                    repaint();
                }
                break;
            }
        }
        return;
    }

    if (isDraggingPlayhead)
    {
        double scrubTime = ((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec;
        setPlayheadPosition(scrubTime);
        return;
    }

    if (isSettingLoop)
    {
        double curTime = ((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec;
        loopEndSec = std::clamp(curTime, loopStartSec + 0.5, totalDurationSec);
        repaint();
        return;
    }

    if (draggingClipId != -1)
    {
        double deltaTime = (e.getDistanceFromDragStartX() / timelineW) * totalDurationSec;
        for (auto& clip : clips)
        {
            if (clip.clipId == draggingClipId)
            {
                if (isResizingClipEnd)
                {
                    clip.durationSec = std::max(0.5, clip.durationSec + deltaTime);
                }
                else
                {
                    clip.startTimeSec = std::max(0.0, dragStartClipTime + deltaTime);
                }
                repaint();
                break;
            }
        }
    }
}

void ArrangementTimelineComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    draggingTrackIdx = -1;
    resizingDividerTrackIdx = -1;

    // Commit Tidal Block Pitch Changes on Drag Release
    if (draggingTidalEventIdx >= 0)
    {
        for (auto& clip : clips)
        {
            if (clip.clipId == selectedClipId || (selectedClipId == -1 && clip.type == ClipType::Pattern))
            {
                std::stringstream ss;
                ss << "[";
                int lastChan = -1;
                for (size_t i = 0; i < clip.cachedEvents.size(); ++i)
                {
                    const auto& ev = clip.cachedEvents[i];
                    if (lastChan >= 0 && ev.channel != lastChan) ss << ", ";
                    else if (i > 0) ss << " ";
                    lastChan = ev.channel;

                    if (ev.isRest) ss << "~";
                    else if (!ev.valueStr.empty()) ss << ev.valueStr;
                    else ss << ev.pitch;
                }
                ss << "]";
                clip.updateTidalPattern(ss.str());
                tidalPatternEditor.setText(clip.tidalPattern);
                commitTidalPattern();
                break;
            }
        }
        draggingTidalEventIdx = -1;
        repaint();
    }

    isDraggingPlayhead = false;
    isSettingLoop = false;
    draggingClipId = -1;
    isResizingClipEnd = false;
}

bool ArrangementTimelineComponent::keyPressed(const juce::KeyPress& key)
{
    bool isCmdOrCtrl = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();

    // Spacebar toggles playback
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        togglePlayback();
        return true;
    }

    // Cmd+D duplicates selected clip
    if (isCmdOrCtrl && (key.getKeyCode() == 'd' || key.getKeyCode() == 'D'))
    {
        duplicateSelectedClip();
        return true;
    }

    // Cmd+B splits clip at playhead
    if (isCmdOrCtrl && (key.getKeyCode() == 'b' || key.getKeyCode() == 'B'))
    {
        splitClipAtPlayhead();
        return true;
    }

    // Backspace/Delete deletes selected clip or event
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey)
    {
        if (selectedClipId != -1)
        {
            deleteClip(selectedClipId);
            return true;
        }
    }

    return false;
}

} // namespace TimeDilationDAW
