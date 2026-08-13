#include "ArrangementTimelineComponent.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

ArrangementTimelineComponent::ArrangementTimelineComponent(RelativisticNodeGraph& graph)
    : nodeGraph(graph)
{
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

    refreshTimeline();
    startTimerHz(30); // 30 FPS timer for playhead animation when playing
}

ArrangementTimelineComponent::~ArrangementTimelineComponent()
{
    stopTimer();
}

void ArrangementTimelineComponent::togglePlayback()
{
    isTimelinePlaying = !isTimelinePlaying;
    playStopButton.setButtonText(isTimelinePlaying ? "STOP" : "PLAY");
    playStopButton.setColour(juce::TextButton::textColourOffId, isTimelinePlaying ? juce::Colours::deeppink : CarbonGoldLookAndFeel::goldAccent);
    repaint();
}

void ArrangementTimelineComponent::rewindToStart()
{
    playheadTimeSec = isLoopEnabled ? loopStartSec : 0.0;
    for (auto& ev : messageEvents) ev.triggeredInCurrentPass = false;
    repaint();
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

    int timelineW = getWidth() - trackHeaderWidth;
    if (timelineW <= 0) return;

    float x = trackHeaderWidth + static_cast<float>(timeSec / totalDurationSec) * timelineW;
    int contentStartY = transportBarHeight + rulerHeight;
    int y = contentStartY + trackIdx * trackHeight + 30;

    juce::String currentText = "";
    if (existingEventId != -1)
    {
        for (const auto& ev : messageEvents)
        {
            if (ev.eventId == existingEventId) { currentText = ev.messageText; break; }
        }
    }
    else
    {
        auto node = nodeGraph.getNode(targetNodeId);
        if (node)
        {
            if (node->getSymbol() == "ladder~") currentText = "cutoff 2000";
            else if (node->getSymbol() == "osc~") currentText = "freq 440";
            else if (node->getSymbol() == "drive~") currentText = "drive 3.5";
            else currentText = "trigger";
        }
    }

    eventEditor.setText(currentText);
    eventEditor.setBounds(static_cast<int>(x), y, 120, 24);
    eventEditor.setVisible(true);
    eventEditor.selectAll();
    eventEditor.grabKeyboardFocus();
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
    editingEventId = -1;
    repaint();
}

void ArrangementTimelineComponent::refreshTimeline()
{
    clips.clear();
    const auto& nodes = nodeGraph.getNodes();
    int trackIdx = 0;
    for (const auto& node : nodes)
    {
        if (node->getSymbol() == "out~") continue;

        TimelineClip c;
        c.clipId = node->getId();
        c.trackIndex = trackIdx++;
        c.startTimeSec = static_cast<double>((c.clipId * 2) % 12);
        c.durationSec = 6.0;
        c.name = juce::String(node->getLabel());

        if (node->getSymbol() == "osc~") c.color = CarbonGoldLookAndFeel::cyberCyan;
        else if (node->getSymbol() == "pluck~") c.color = CarbonGoldLookAndFeel::goldAccent;
        else if (node->getSymbol() == "ladder~") c.color = CarbonGoldLookAndFeel::royalViolet;
        else if (node->getSymbol() == "time.warp") c.color = juce::Colours::deeppink;
        else c.color = juce::Colours::darkgrey;

        clips.push_back(c);
    }

    // Add demo timeline message events if graph has active nodes
    if (messageEvents.empty() && !nodes.empty())
    {
        int tIdx = 0;
        for (const auto& node : nodes)
        {
            if (node->getSymbol() == "out~") continue;
            if (node->getSymbol() == "ladder~") addMessageEvent(node->getId(), tIdx, 4.0, "cutoff 2200");
            else if (node->getSymbol() == "osc~") addMessageEvent(node->getId(), tIdx, 8.0, "freq 550");
            else if (node->getSymbol() == "drive~") addMessageEvent(node->getId(), tIdx, 12.0, "drive 3.5");
            tIdx++;
        }
    }

    repaint();
}

void ArrangementTimelineComponent::setPlayheadPosition(double timeInSeconds)
{
    playheadTimeSec = timeInSeconds;
    repaint();
}

void ArrangementTimelineComponent::timerCallback()
{
    double prevTimeSec = playheadTimeSec;

    if (isTimelinePlaying)
    {
        playheadTimeSec += 1.0 / 30.0;

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
    }

    if (playheadTimeSec < prevTimeSec)
    {
        for (auto& ev : messageEvents) ev.triggeredInCurrentPass = false;
    }

    // Real-time Event Dispatch Engine during Playback!
    if (isTimelinePlaying)
    {
        for (auto& ev : messageEvents)
        {
            if (!ev.triggeredInCurrentPass && playheadTimeSec >= ev.timeSec)
            {
                ev.triggeredInCurrentPass = true;
                auto destNode = nodeGraph.getNode(ev.targetNodeId);
                if (destNode)
                {
                    destNode->receiveMessage(ev.messageText.toStdString());
                }
            }
        }
    }

    // Query JUCE AudioPlayHead PositionInfo metrics
    double bpm = 120.0;
    int numBeats = 4;
    int beatValue = 4;

    if (auto* ph = nodeGraph.getAudioPlayHead())
    {
        if (auto posOpt = ph->getPosition())
        {
            auto pos = *posOpt;
            if (pos.getBpm()) bpm = *pos.getBpm();
            if (pos.getTimeSignature())
            {
                numBeats = pos.getTimeSignature()->numerator;
                beatValue = pos.getTimeSignature()->denominator;
            }
        }
    }

    // Calculate PPQ (Pulses Per Quarter Note) and Bar.Beat position from JUCE PositionInfo metrics
    double ppq = playheadTimeSec * (bpm / 60.0);
    double quarterNotesPerBeat = 4.0 / static_cast<double>(beatValue);
    double quarterNotesPerBar = static_cast<double>(numBeats) * quarterNotesPerBeat;

    int currentBar = 1 + static_cast<int>(std::floor(ppq / std::max(0.1, quarterNotesPerBar)));
    double ppqInBar = std::fmod(ppq, quarterNotesPerBar);
    int currentBeat = 1 + static_cast<int>(std::floor(ppqInBar / std::max(0.1, quarterNotesPerBeat)));

    int mins = static_cast<int>(playheadTimeSec) / 60;
    int secs = static_cast<int>(playheadTimeSec) % 60;
    int ms = static_cast<int>((playheadTimeSec - std::floor(playheadTimeSec)) * 100.0);

    char buf[80];
    std::snprintf(buf, sizeof(buf), "Bar %d.%d (%d/%d) | %02d:%02d.%02d", currentBar, currentBeat, numBeats, beatValue, mins, secs, ms);
    timeDisplayLabel.setText(buf, juce::dontSendNotification);

    repaint();
}

bool ArrangementTimelineComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        togglePlayback();
        return true;
    }
    else if (key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey)
    {
        if (selectedEventId != -1)
        {
            deleteMessageEvent(selectedEventId);
            return true;
        }
    }
    return false;
}

void ArrangementTimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto pos = e.position;
    int timelineW = getWidth() - trackHeaderWidth;
    if (timelineW <= 0) return;

    if (isEditingEvent && !eventEditor.getBounds().contains(pos.toInt()))
    {
        commitEventEditor();
    }

    int contentStartY = transportBarHeight + rulerHeight;

    // Check if clicked an Event Badge
    for (const auto& ev : messageEvents)
    {
        int y = contentStartY + ev.trackIndex * trackHeight + 30;
        float x = trackHeaderWidth + static_cast<float>(ev.timeSec / totalDurationSec) * timelineW;
        juce::Rectangle<float> badgeRect(x, static_cast<float>(y), 110.0f, 22.0f);

        if (badgeRect.contains(pos))
        {
            selectedEventId = ev.eventId;
            isDraggingEvent = true;

            // Double Click Event Badge -> Edit Text
            if (e.getNumberOfClicks() >= 2)
            {
                spawnEventEditor(ev.targetNodeId, ev.trackIndex, ev.timeSec, ev.eventId);
                return;
            }

            // Right Click Event Badge -> Context Menu
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu m;
                m.addItem(1, "Edit Message Payload...");
                m.addItem(2, "Duplicate Message Event");
                m.addSeparator();
                m.addItem(3, "Delete Event");
                m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, ev](int result) {
                    if (result == 1) spawnEventEditor(ev.targetNodeId, ev.trackIndex, ev.timeSec, ev.eventId);
                    else if (result == 2) addMessageEvent(ev.targetNodeId, ev.trackIndex, std::min(totalDurationSec, ev.timeSec + 2.0), ev.messageText);
                    else if (result == 3) deleteMessageEvent(ev.eventId);
                });
                return;
            }
            repaint();
            return;
        }
    }

    selectedEventId = -1;

    // Check if clicked inside Ruler Area
    if (pos.y >= transportBarHeight && pos.y < (transportBarHeight + rulerHeight) && pos.x >= trackHeaderWidth)
    {
        double clickedSec = std::clamp(static_cast<double>((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec, 0.0, totalDurationSec);

        if (e.mods.isShiftDown())
        {
            isSettingLoop = true;
            loopStartSec = clickedSec;
            loopEndSec = std::min(totalDurationSec, clickedSec + 4.0);
        }
        else
        {
            playheadTimeSec = clickedSec;
        }
        repaint();
        return;
    }

    // Check if double-clicked inside Track Lane -> Spawn New Message Event!
    if (pos.y >= contentStartY && pos.x >= trackHeaderWidth)
    {
        int clickedTrackIdx = static_cast<int>((pos.y - contentStartY) / trackHeight);
        const auto& nodes = nodeGraph.getNodes();

        int currentTrackIdx = 0;
        for (const auto& node : nodes)
        {
            if (node->getSymbol() == "out~") continue;
            if (currentTrackIdx == clickedTrackIdx)
            {
                double clickedSec = std::clamp(static_cast<double>((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec, 0.0, totalDurationSec);
                if (e.getNumberOfClicks() >= 2)
                {
                    spawnEventEditor(node->getId(), clickedTrackIdx, clickedSec, -1);
                    return;
                }
                break;
            }
            currentTrackIdx++;
        }
    }
    repaint();
}

void ArrangementTimelineComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    int timelineW = getWidth() - trackHeaderWidth;
    if (timelineW <= 0) return;

    if (isDraggingEvent && selectedEventId != -1 && pos.x >= trackHeaderWidth)
    {
        double currentSec = std::clamp(static_cast<double>((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec, 0.0, totalDurationSec);
        for (auto& ev : messageEvents)
        {
            if (ev.eventId == selectedEventId)
            {
                ev.timeSec = currentSec;
                break;
            }
        }
        repaint();
        return;
    }

    if (isSettingLoop && pos.x >= trackHeaderWidth)
    {
        double currentSec = std::clamp(static_cast<double>((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec, 0.0, totalDurationSec);
        if (currentSec > loopStartSec)
        {
            loopEndSec = currentSec;
        }
        else
        {
            loopEndSec = loopStartSec;
            loopStartSec = currentSec;
        }
        repaint();
    }
}

void ArrangementTimelineComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isSettingLoop = false;
    isDraggingEvent = false;
}

void ArrangementTimelineComponent::paint(juce::Graphics& g)
{
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    int timelineW = getWidth() - trackHeaderWidth;
    if (timelineW <= 0) return;

    // 1. Top Transport Header Bar Background
    g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.2f));
    g.fillRect(0, 0, getWidth(), transportBarHeight);
    g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.2f));
    g.drawHorizontalLine(transportBarHeight - 1, 0.0f, static_cast<float>(getWidth()));

    // 2. Timeline Ruler Bar
    int rulerY = transportBarHeight;
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, rulerY, getWidth(), rulerHeight);
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
    g.drawHorizontalLine(rulerY + rulerHeight, 0.0f, static_cast<float>(getWidth()));

    // Ruler Ticks (Bar 1..32 / Time 0s..30s)
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    for (int bar = 0; bar <= 16; ++bar)
    {
        float x = trackHeaderWidth + (bar / 16.0f) * timelineW;
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(rulerY), static_cast<float>(rulerY + rulerHeight));
        g.drawText("Bar " + juce::String(bar * 2 + 1), static_cast<int>(x) + 4, rulerY + 4, 50, 16, juce::Justification::left);
    }

    // 3. Render Loop Region Overlay (Translucent Gold Box)
    if (isLoopEnabled && loopEndSec > loopStartSec)
    {
        float loopX1 = trackHeaderWidth + static_cast<float>(loopStartSec / totalDurationSec) * timelineW;
        float loopX2 = trackHeaderWidth + static_cast<float>(loopEndSec / totalDurationSec) * timelineW;

        juce::Rectangle<float> loopRect(loopX1, static_cast<float>(rulerY), loopX2 - loopX1, static_cast<float>(getHeight() - rulerY));
        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.12f));
        g.fillRect(loopRect);

        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.drawVerticalLine(static_cast<int>(loopX1), static_cast<float>(rulerY), static_cast<float>(getHeight()));
        g.drawVerticalLine(static_cast<int>(loopX2), static_cast<float>(rulerY), static_cast<float>(getHeight()));

        g.setFont(10.0f);
        g.drawText("LOOP IN", static_cast<int>(loopX1) + 4, rulerY + 16, 50, 14, juce::Justification::left);
        g.drawText("LOOP OUT", static_cast<int>(loopX2) - 54, rulerY + 16, 50, 14, juce::Justification::right);
    }

    // 4. Track Lanes Backgrounds & Headers
    const auto& nodes = nodeGraph.getNodes();
    int trackIdx = 0;
    int contentStartY = transportBarHeight + rulerHeight;
    for (const auto& node : nodes)
    {
        if (node->getSymbol() == "out~") continue;
        int y = contentStartY + trackIdx * trackHeight;

        // Track Header Box
        g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.05f));
        g.fillRect(0, y, trackHeaderWidth - 2, trackHeight - 2);

        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("Tr " + juce::String(trackIdx + 1) + ": " + juce::String(node->getLabel()), 10, y + 8, trackHeaderWidth - 20, 18, juce::Justification::left);

        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        g.drawText("Node #" + juce::String(node->getId()) + " [" + juce::String(node->getSymbol()) + "]", 10, y + 30, trackHeaderWidth - 20, 16, juce::Justification::left);

        // Track Lane Grid Line
        g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
        g.drawHorizontalLine(y + trackHeight - 1, static_cast<float>(trackHeaderWidth), static_cast<float>(getWidth()));

        trackIdx++;
    }

    // 5. Render Audio / MIDI Timeline Clip Blocks
    for (const auto& c : clips)
    {
        int y = contentStartY + c.trackIndex * trackHeight + 4;
        float startX = trackHeaderWidth + static_cast<float>(c.startTimeSec / totalDurationSec) * timelineW;
        float clipW = static_cast<float>(c.durationSec / totalDurationSec) * timelineW;

        juce::Rectangle<float> clipRect(startX, static_cast<float>(y), clipW, static_cast<float>(trackHeight - 10));

        g.setColour(c.color.withAlpha(0.25f));
        g.fillRoundedRectangle(clipRect, 4.0f);

        g.setColour(c.color);
        g.drawRoundedRectangle(clipRect, 4.0f, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(c.name, clipRect.reduced(6.0f), juce::Justification::topLeft, true);
    }

    // 6. Render Message Event Markers (Gold Flag Badges)
    for (const auto& ev : messageEvents)
    {
        int y = contentStartY + ev.trackIndex * trackHeight + 30;
        float x = trackHeaderWidth + static_cast<float>(ev.timeSec / totalDurationSec) * timelineW;
        bool isSel = (selectedEventId == ev.eventId);

        juce::Rectangle<float> badgeRect(x, static_cast<float>(y), 110.0f, 22.0f);

        g.setColour(isSel ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
        g.fillRoundedRectangle(badgeRect, 3.0f);

        g.setColour(isSel ? juce::Colours::white : CarbonGoldLookAndFeel::goldAccent);
        g.drawRoundedRectangle(badgeRect, 3.0f, isSel ? 2.0f : 1.2f);

        g.setColour(isSel ? juce::Colours::black : CarbonGoldLookAndFeel::goldAccent);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("MSG: " + ev.messageText, badgeRect.reduced(4.0f, 1.0f), juce::Justification::centredLeft, true);
    }

    // 7. Moving Playhead Scrubber Line
    float playheadX = trackHeaderWidth + static_cast<float>(playheadTimeSec / totalDurationSec) * timelineW;
    g.setColour(CarbonGoldLookAndFeel::goldAccent);
    g.drawVerticalLine(static_cast<int>(playheadX), static_cast<float>(rulerY), static_cast<float>(getHeight()));

    // Playhead Header Triangle
    juce::Path p;
    p.addTriangle(playheadX - 6.0f, static_cast<float>(rulerY), playheadX + 6.0f, static_cast<float>(rulerY), playheadX, static_cast<float>(rulerY + 10));
    g.fillPath(p);
}

void ArrangementTimelineComponent::resized()
{
    playStopButton.setBounds(6, 4, 80, 24);
    rewindButton.setBounds(90, 4, 80, 24);
    loopButton.setBounds(174, 4, 90, 24);
    timeDisplayLabel.setBounds(272, 4, 200, 24);
}

} // namespace TimeDilationDAW
