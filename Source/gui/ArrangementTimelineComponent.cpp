#include "ArrangementTimelineComponent.h"
#include "CarbonGoldLookAndFeel.h"
#include <iomanip>
#include <sstream>

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

    addClipButton.onClick = [this]() {
        addClip(0, playheadTimeSec, 4.0, "Pattern Clip", ClipType::Pattern);
    };
    addClipButton.setColour(juce::TextButton::buttonColourId, CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
    addClipButton.setColour(juce::TextButton::textColourOffId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(addClipButton);

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

    refreshTimeline();
    startTimerHz(30);
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

    clips.push_back(clip);
    selectedClipId = clip.clipId;
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

        // Check and dispatch scheduled timeline message events
        for (auto& ev : messageEvents)
        {
            if (!ev.triggeredInCurrentPass && playheadTimeSec >= ev.timeSec && lastPlayheadTimeSec <= ev.timeSec)
            {
                ev.triggeredInCurrentPass = true;
                auto node = nodeGraph.getNode(ev.targetNodeId);
                if (node)
                {
                    node->receiveMessage(ev.messageText.toStdString());
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
    const auto& nodes = nodeGraph.getNodes();
    int numTracks = std::max(4, static_cast<int>(nodes.size()));

    int tracksVisibleH = getHeight() - yStart - (isPianoRollVisible ? pianoRollHeight : 0);

    for (int t = 0; t < numTracks; ++t)
    {
        int y = yStart + t * trackHeight;
        if (y + trackHeight > yStart + tracksVisibleH) break;

        // Track Header Panel
        auto headerR = juce::Rectangle<float>(0.0f, static_cast<float>(y), static_cast<float>(trackHeaderWidth), static_cast<float>(trackHeight));
        g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.1f));
        g.fillRect(headerR);

        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
        g.drawRect(headerR, 1.0f);

        juce::String trackLabel = "Track " + juce::String(t + 1);
        juce::String nodeSym = "Pattern";
        if (t < static_cast<int>(nodes.size()))
        {
            trackLabel = nodes[static_cast<size_t>(t)]->getLabel();
            nodeSym = nodes[static_cast<size_t>(t)]->getSymbol();
        }

        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.drawText(trackLabel, 10, y + 8, trackHeaderWidth - 20, 18, juce::Justification::left);

        g.setFont(juce::Font(10.0f, juce::Font::italic));
        g.setColour(CarbonGoldLookAndFeel::cyberCyan);
        g.drawText(nodeSym, 10, y + 26, trackHeaderWidth - 20, 16, juce::Justification::left);

        // Track Lane Background
        auto laneR = juce::Rectangle<float>(static_cast<float>(trackHeaderWidth), static_cast<float>(y), timelineW, static_cast<float>(trackHeight));
        g.setColour((t % 2 == 0) ? CarbonGoldLookAndFeel::carbonBg.brighter(0.02f) : CarbonGoldLookAndFeel::carbonBg);
        g.fillRect(laneR);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine(y + trackHeight, static_cast<float>(trackHeaderWidth), b.getWidth());
    }

    // 5. Draw Timeline Clips
    for (const auto& clip : clips)
    {
        int y = yStart + clip.trackIndex * trackHeight;
        if (y + trackHeight > yStart + tracksVisibleH) continue;

        float cx = static_cast<float>(trackHeaderWidth) + static_cast<float>((clip.startTimeSec / totalDurationSec) * timelineW);
        float cw = static_cast<float>((clip.durationSec / totalDurationSec) * timelineW);
        auto clipR = juce::Rectangle<float>(cx, static_cast<float>(y + 4), cw, static_cast<float>(trackHeight - 8));

        bool isSelected = (clip.clipId == selectedClipId);

        g.setColour(clip.color.withAlpha(isSelected ? 0.35f : 0.20f));
        g.fillRoundedRectangle(clipR, 4.0f);

        g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : clip.color);
        g.drawRoundedRectangle(clipR, 4.0f, isSelected ? 2.0f : 1.0f);

        // Clip Title
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(clip.name, static_cast<int>(cx + 6), y + 6, static_cast<int>(cw - 12), 16, juce::Justification::left);

        // Miniature note / automation preview
        if (clip.type == ClipType::Pattern)
        {
            int numSteps = static_cast<int>(clip.stepPitches.size());
            float stepW = cw / static_cast<float>(numSteps);
            for (int s = 0; s < numSteps; ++s)
            {
                if (s < static_cast<int>(clip.stepGates.size()) && clip.stepGates[static_cast<size_t>(s)])
                {
                    float noteNorm = (clip.stepPitches[static_cast<size_t>(s)] - 36) / 48.0f;
                    float ny = clipR.getBottom() - 4.0f - noteNorm * (clipR.getHeight() - 24.0f);
                    g.setColour(clip.color);
                    g.fillRect(cx + s * stepW + 1.0f, ny, stepW - 2.0f, 4.0f);
                }
            }
        }
        else if (clip.type == ClipType::Automation)
        {
            drawAutomationCurves(g, clip, clipR);
        }
    }

    // 6. Draw Scheduled Message Events
    for (const auto& ev : messageEvents)
    {
        int y = yStart + ev.trackIndex * trackHeight;
        if (y + trackHeight > yStart + tracksVisibleH) continue;

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

    // 8. Piano Roll & Step Grid Drawer
    if (isPianoRollVisible)
    {
        auto drawerBounds = juce::Rectangle<float>(0.0f, b.getHeight() - pianoRollHeight, b.getWidth(), static_cast<float>(pianoRollHeight));
        drawPianoRollDrawer(g, drawerBounds);
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
    const TimelineClip* activeClip = nullptr;
    for (const auto& c : clips)
    {
        if (c.clipId == selectedClipId) { activeClip = &c; clipName = c.name; break; }
    }

    g.drawText("PIANO ROLL / 16-STEP GRID: " + clipName, 12, static_cast<int>(bounds.getY() + 4), 300, 20, juce::Justification::left);

    if (!activeClip) return;

    // Draw 16 Step Buttons
    int numSteps = 16;
    float stepAreaW = bounds.getWidth() - 24.0f;
    float stepW = stepAreaW / static_cast<float>(numSteps);
    float gridY = bounds.getY() + 30.0f;
    float gridH = bounds.getHeight() - 65.0f;

    for (int s = 0; s < numSteps; ++s)
    {
        auto stepR = juce::Rectangle<float>(12.0f + s * stepW, gridY, stepW - 4.0f, gridH);
        bool hasGate = (s < static_cast<int>(activeClip->stepGates.size())) && activeClip->stepGates[static_cast<size_t>(s)];

        g.setColour(hasGate ? CarbonGoldLookAndFeel::goldAccent.withAlpha(0.85f) : CarbonGoldLookAndFeel::slatePanel);
        g.fillRoundedRectangle(stepR, 3.0f);

        g.setColour(hasGate ? CarbonGoldLookAndFeel::carbonBg : juce::Colours::dimgrey);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(juce::String(s + 1), stepR.withHeight(16.0f), juce::Justification::centred);

        if (hasGate && s < static_cast<int>(activeClip->stepPitches.size()))
        {
            int pitch = activeClip->stepPitches[static_cast<size_t>(s)];
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText(juce::String(pitch), stepR.reduced(2.0f), juce::Justification::centred);
        }

        // Velocity Bar at bottom
        float vel = (s < static_cast<int>(activeClip->stepVelocities.size())) ? activeClip->stepVelocities[static_cast<size_t>(s)] : 0.8f;
        float velY = bounds.getBottom() - 25.0f;
        g.setColour(CarbonGoldLookAndFeel::cyberCyan);
        g.fillRect(12.0f + s * stepW, velY + (1.0f - vel) * 18.0f, stepW - 4.0f, vel * 18.0f);
    }
}

void ArrangementTimelineComponent::resized()
{
    int y = 4;
    int btnH = 26;

    playStopButton.setBounds(10, y, 70, btnH);
    rewindButton.setBounds(85, y, 75, btnH);
    loopButton.setBounds(165, y, 80, btnH);
    addClipButton.setBounds(250, y, 85, btnH);
    togglePianoRollBtn.setBounds(340, y, 95, btnH);

    timeDisplayLabel.setBounds(getWidth() - 240, y, 230, btnH);
}

void ArrangementTimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);

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

    // 2. Clicked in Piano Roll Drawer: Toggle step gates or edit pitches
    if (isPianoRollVisible && pos.y >= getHeight() - pianoRollHeight)
    {
        float gridY = getHeight() - pianoRollHeight + 30.0f;
        float stepAreaW = getWidth() - 24.0f;
        float stepW = stepAreaW / 16.0f;

        int stepIdx = static_cast<int>((pos.x - 12.0f) / stepW);
        if (stepIdx >= 0 && stepIdx < 16)
        {
            for (auto& clip : clips)
            {
                if (clip.clipId == selectedClipId)
                {
                    if (clip.stepGates.size() <= static_cast<size_t>(stepIdx)) clip.stepGates.resize(16, false);
                    if (clip.stepPitches.size() <= static_cast<size_t>(stepIdx)) clip.stepPitches.resize(16, 60);
                    if (clip.stepVelocities.size() <= static_cast<size_t>(stepIdx)) clip.stepVelocities.resize(16, 0.85f);

                    clip.stepGates[static_cast<size_t>(stepIdx)] = !clip.stepGates[static_cast<size_t>(stepIdx)];
                    selectedStepIndex = stepIdx;
                    repaint();
                    break;
                }
            }
        }
        return;
    }

    // 3. Clicked in Tracks Area: Select / Drag Clips or Messages
    int yStart = transportBarHeight + rulerHeight;
    int tracksVisibleH = getHeight() - yStart - (isPianoRollVisible ? pianoRollHeight : 0);

    for (const auto& clip : clips)
    {
        int y = yStart + clip.trackIndex * trackHeight;
        if (y + trackHeight > yStart + tracksVisibleH) continue;

        float cx = static_cast<float>(trackHeaderWidth) + static_cast<float>((clip.startTimeSec / totalDurationSec) * timelineW);
        float cw = static_cast<float>((clip.durationSec / totalDurationSec) * timelineW);
        auto clipR = juce::Rectangle<float>(cx, static_cast<float>(y + 4), cw, static_cast<float>(trackHeight - 8));

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

void ArrangementTimelineComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);

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
    isDraggingPlayhead = false;
    isSettingLoop = false;
    draggingClipId = -1;
    isResizingClipEnd = false;
}

void ArrangementTimelineComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.position;
    float timelineW = static_cast<float>(getWidth() - trackHeaderWidth);

    if (pos.x > trackHeaderWidth && pos.y > transportBarHeight + rulerHeight)
    {
        int yStart = transportBarHeight + rulerHeight;
        int trackIdx = static_cast<int>((pos.y - yStart) / trackHeight);
        double clickTime = ((pos.x - trackHeaderWidth) / timelineW) * totalDurationSec;

        addClip(trackIdx, clickTime, 4.0, "Sequence Clip " + juce::String(clips.size() + 1), ClipType::Pattern);
    }
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
