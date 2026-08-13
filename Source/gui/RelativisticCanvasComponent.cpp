#include "RelativisticCanvasComponent.h"
#include "CarbonGoldLookAndFeel.h"
#include "../dsp/CompositeNode.h"
#include "../dsp/RelativisticNodeFactory.h"

namespace TimeDilationDAW
{

RelativisticCanvasComponent::RelativisticCanvasComponent(RelativisticNodeGraph& graph)
    : rootGraph(graph)
{
    graphStack.push_back(&rootGraph);
    breadcrumbs.push_back("Master Patch");

    setWantsKeyboardFocus(true);

    objectEditor.setMultiLine(false);
    objectEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::slatePanel);
    objectEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    objectEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::cyberCyan);
    objectEditor.setColour(juce::TextEditor::focusedOutlineColourId, CarbonGoldLookAndFeel::goldAccent);

    objectEditor.onReturnKey = [this]() { commitObjectCreation(); };
    objectEditor.onEscapeKey = [this]() { cancelObjectCreation(); };

    addChildComponent(objectEditor);

    recenterButton.setButtonText("Recenter View");
    recenterButton.onClick = [this]() { recenterView(); };
    addAndMakeVisible(recenterButton);

    startTimerHz(60); // 60 FPS real-time scope & canvas repainting
}

RelativisticCanvasComponent::~RelativisticCanvasComponent()
{
    stopTimer();
}

RelativisticNodeGraph& RelativisticCanvasComponent::getCurrentGraph()
{
    if (!graphStack.empty()) return *graphStack.back();
    return rootGraph;
}

void RelativisticCanvasComponent::pushSubGraphView(RelativisticNodeGraph* subGraph, const std::string& name)
{
    if (subGraph)
    {
        graphStack.push_back(subGraph);
        breadcrumbs.push_back(name);
        clearSelection();
        repaint();
    }
}

void RelativisticCanvasComponent::popSubGraphView()
{
    if (graphStack.size() > 1)
    {
        graphStack.pop_back();
        breadcrumbs.pop_back();
        clearSelection();
        repaint();
    }
}

juce::Rectangle<float> RelativisticCanvasComponent::getNodeBounds(const RelativisticNode& node) const
{
    float fontWidth = juce::Font(12.0f, juce::Font::bold).getStringWidthFloat(node.getLabel());
    float minWidthForPorts = static_cast<float>(std::max(node.getInlets().size(), node.getOutlets().size()) + 1) * 24.0f;
    float calculatedW = std::max({ node.width, fontWidth + 85.0f, minWidthForPorts, 130.0f });

    float baseHeight = node.showRealtimeDisplay ? 85.0f : 45.0f;
    float calculatedH = std::max(node.height, baseHeight);

    return { node.xPos + viewOffsetX, node.yPos + viewOffsetY, calculatedW, calculatedH };
}

juce::Point<float> RelativisticCanvasComponent::getPortPos(const RelativisticNode& node, bool isOutlet, int portIdx) const
{
    auto b = getNodeBounds(node);
    const auto& ports = isOutlet ? node.getOutlets() : node.getInlets();
    int count = static_cast<int>(ports.size());
    float spacing = b.getWidth() / (count + 1);
    float x = b.getX() + spacing * (portIdx + 1);
    float y = isOutlet ? b.getBottom() : b.getY();
    return { x, y };
}

void RelativisticCanvasComponent::paint(juce::Graphics& g)
{
    auto& currGraph = getCurrentGraph();

    // Background Grid
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    g.setColour(juce::Colour::fromRGB(0x1a, 0x1a, 0x24));
    for (int x = 0; x < getWidth(); x += 20)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    for (int y = 0; y < getHeight(); y += 20)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    // Breadcrumb Bar
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, 0, getWidth(), 28);
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
    g.drawHorizontalLine(28, 0.0f, static_cast<float>(getWidth()));

    juce::String bcStr = "";
    for (size_t i = 0; i < breadcrumbs.size(); ++i)
    {
        if (i > 0) bcStr += "  >  ";
        bcStr += juce::String(breadcrumbs[i]);
    }
    g.setColour(CarbonGoldLookAndFeel::goldAccent);
    g.setFont(12.0f);
    g.drawText(bcStr, 10, 4, getWidth() - 20, 20, juce::Justification::left, true);

    // Render Patch Connections
    for (const auto& conn : currGraph.getConnections())
    {
        auto srcNode = currGraph.getNode(conn.sourceNodeId);
        auto destNode = currGraph.getNode(conn.destNodeId);
        if (!srcNode || !destNode) continue;

        auto p1 = getPortPos(*srcNode, true, conn.sourcePortIndex);
        auto p2 = getPortPos(*destNode, false, conn.destPortIndex);

        juce::Path cablePath;
        cablePath.startNewSubPath(p1);
        cablePath.cubicTo(p1.x, p1.y + 40.0f, p2.x, p2.y - 40.0f, p2.x, p2.y);

        bool isSelectedCable = (conn.connectionId == selectedConnectionId);
        juce::Colour typeColor = (conn.dataType == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                                ((conn.dataType == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);

        juce::Colour cableColor = conn.isFeedbackCycle ? juce::Colour::fromRGB(0xff, 0x44, 0x44)
                                                       : (isSelectedCable ? CarbonGoldLookAndFeel::goldAccent : typeColor);

        g.setColour(cableColor.withAlpha(0.35f));
        g.strokePath(cablePath, juce::PathStrokeType(5.0f));

        g.setColour(cableColor);
        g.strokePath(cablePath, juce::PathStrokeType(isSelectedCable ? 2.5f : 1.5f));

        // Real-Time Animated Kinetic Signal Pulse Dots & Flash Animations along the Cable!
        double millis = juce::Time::getMillisecondCounterHiRes();
        double seconds = millis * 0.001;

        if (conn.dataType == PortDataType::Message)
        {
            // Flash brightness pulse when a message passes through the cable!
            double age = seconds - conn.lastMessageTriggerTime;
            if (age >= 0.0 && age < 0.45)
            {
                float flashAlpha = 1.0f - static_cast<float>(age / 0.45);
                g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(flashAlpha * 0.8f));
                g.strokePath(cablePath, juce::PathStrokeType(4.0f));

                float pulsePos = static_cast<float>(age / 0.45);
                auto dot = cablePath.getPointAlongPath(pulsePos * cablePath.getLength());
                g.setColour(juce::Colours::white.withAlpha(flashAlpha));
                g.fillEllipse(dot.x - 4.0f, dot.y - 4.0f, 8.0f, 8.0f);
            }
        }
        else // Continuous audio / time signals: subtle, non-distracting slow flow
        {
            double speedFactor = conn.isFeedbackCycle ? 0.0006 : 0.00025;
            float pulsePos = static_cast<float>(std::fmod(millis * speedFactor, 1.0));
            auto dot = cablePath.getPointAlongPath(pulsePos * cablePath.getLength());

            g.setColour(typeColor.withAlpha(0.35f));
            g.fillEllipse(dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
        }

        if (conn.isFeedbackCycle)
        {
            auto midPt = cablePath.getPointAlongPath(0.5f * cablePath.getLength());
            g.setColour(juce::Colour::fromRGB(0xff, 0x44, 0x44));
            g.setFont(10.0f);
            g.drawText("1-blk z⁻¹", juce::Rectangle<float>(midPt.x - 25, midPt.y - 12, 50, 14), juce::Justification::centred, false);
        }
    }

    // Active drag cable
    if (draggingPortNodeId != -1)
    {
        auto node = currGraph.getNode(draggingPortNodeId);
        if (node)
        {
            auto p1 = getPortPos(*node, isDraggingFromOutlet, draggingPortIdx);
            juce::Path dragCable;
            dragCable.startNewSubPath(p1);
            dragCable.cubicTo(p1.x, p1.y + (isDraggingFromOutlet ? 40.0f : -40.0f),
                              dragCurrentPos.x, dragCurrentPos.y + (isDraggingFromOutlet ? -40.0f : 40.0f),
                              dragCurrentPos.x, dragCurrentPos.y);

            PortDataType dragType = PortDataType::Audio;
            if (isDraggingFromOutlet && draggingPortIdx < static_cast<int>(node->getOutlets().size()))
                dragType = node->getOutlets()[static_cast<size_t>(draggingPortIdx)].dataType;
            else if (!isDraggingFromOutlet && draggingPortIdx < static_cast<int>(node->getInlets().size()))
                dragType = node->getInlets()[static_cast<size_t>(draggingPortIdx)].dataType;

            juce::Colour dragColor = (dragType == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                                    ((dragType == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);

            g.setColour(dragColor);
            g.strokePath(dragCable, juce::PathStrokeType(2.5f));
        }
    }

    // Render Nodes
    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        bool isSelected = isNodeSelected(node->getId());

        // Paint Nodes
        g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : CarbonGoldLookAndFeel::slatePanel);
        g.fillRoundedRectangle(b, 5.0f);

        g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.4f));
        g.drawRoundedRectangle(b, 5.0f, isSelected ? 2.5f : 1.0f);

        // Header Title Label with Wrapping Support
        auto headerRect = b.removeFromTop(22.0f);

        // Realtime Display Toggle Button [👁]
        auto toggleBtnRect = headerRect.removeFromRight(22.0f).reduced(2.0f);

        // Scope Mode Toggle Button
        auto modeBtnRect = headerRect.removeFromRight(46.0f).reduced(2.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawFittedText(node->getLabel(), headerRect.reduced(6.0f, 0.0f).toNearestInt(), juce::Justification::left, 2, 0.9f);

        g.setColour(node->showRealtimeDisplay ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::grey);
        g.drawRoundedRectangle(toggleBtnRect, 3.0f, 1.0f);
        g.setFont(10.0f);
        g.drawText("👁", toggleBtnRect, juce::Justification::centred, false);

        g.setColour((node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform) ? CarbonGoldLookAndFeel::cyberCyan : CarbonGoldLookAndFeel::royalViolet);
        g.drawRoundedRectangle(modeBtnRect, 3.0f, 1.0f);
        g.setFont(9.0f);
        juce::String modeStr;
        if (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
        {
            modeStr = "Audio";
        }
        else
        {
            if (node->timeVarMode == RelativisticNode::TimeScopeVariable::SpeedGamma) modeStr = "Speed";
            else if (node->timeVarMode == RelativisticNode::TimeScopeVariable::OffsetTau) modeStr = "Offset";
            else if (node->timeVarMode == RelativisticNode::TimeScopeVariable::CouplingC) modeStr = "Flex";
            else modeStr = "Multi";
        }
        g.drawText(modeStr, modeBtnRect, juce::Justification::centred, false);

        // Realtime Scope & Value Display Area
        if (node->showRealtimeDisplay && b.getHeight() > 30.0f)
        {
            auto scopeBox = b.reduced(4.0f, 4.0f);
            g.setColour(juce::Colour::fromRGB(0x10, 0x12, 0x18));
            g.fillRoundedRectangle(scopeBox, 3.0f);
            g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.3f));
            g.drawRoundedRectangle(scopeBox, 3.0f, 1.0f);

            // Fetch live TimePolyFrame data from first time inlet or outlet
            const TimePolyFrame* frame = nullptr;
            if (!node->getInlets().empty() && node->getInlets()[0].dataType == PortDataType::Time)
                frame = &node->getInletTimeFrame(0);
            else if (!node->getOutlets().empty() && node->getOutlets()[0].dataType == PortDataType::Time)
                frame = &node->getOutletTimeFrame(0);

            double currentGamma = frame ? frame->masterGamma : 1.0;
            double currentTau = (frame && !frame->streams.empty()) ? frame->streams[0].tau : 0.0;
            double tHiRes = juce::Time::getMillisecondCounterHiRes() * 0.001;

            if (auto outNode = std::dynamic_pointer_cast<OutNode>(node))
            {
                float rmsL = outNode->getRmsL();
                float rmsR = outNode->getRmsR();
                const auto& waveBuf = outNode->getWaveformBuffer();
                size_t writeIdx = outNode->getWaveformWritePos();

                // Draw Waveform Oscilloscope inside scopeBox
                auto waveArea = scopeBox.withTrimmedRight(28.0f);
                g.setColour(juce::Colour::fromRGB(0x0a, 0x0c, 0x10));
                g.fillRect(waveArea);
                g.setColour(CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.25f));
                g.drawHorizontalLine(static_cast<int>(waveArea.getCentreY()), waveArea.getX(), waveArea.getRight());

                if (!waveBuf.empty())
                {
                    juce::Path wavePath;
                    float midY = waveArea.getCentreY();
                    float h = waveArea.getHeight() * 0.45f;
                    float w = waveArea.getWidth();
                    size_t len = waveBuf.size();
                    size_t readStart = (writeIdx + len - 128) % len;

                    for (int i = 0; i < 128; ++i)
                    {
                        float val = waveBuf[(readStart + i) % len];
                        float px = waveArea.getX() + (i / 128.0f) * w;
                        float py = midY - val * h;
                        if (i == 0) wavePath.startNewSubPath(px, py);
                        else wavePath.lineTo(px, py);
                    }
                    g.setColour(CarbonGoldLookAndFeel::goldAccent);
                    g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                }

                // Draw Dual L/R RMS Meters on the right side
                auto meterArea = scopeBox.withLeft(waveArea.getRight() + 2.0f);
                auto lMeter = meterArea.removeFromLeft(11.0f).reduced(1.0f, 2.0f);
                auto rMeter = meterArea.removeFromLeft(11.0f).reduced(1.0f, 2.0f);

                auto drawMeter = [&](juce::Rectangle<float> rect, float level) {
                    g.setColour(juce::Colours::black);
                    g.fillRect(rect);
                    float fillH = rect.getHeight() * std::clamp(level * 2.0f, 0.0f, 1.0f);
                    auto fillRect = rect.withHeight(fillH).withY(rect.getBottom() - fillH);
                    juce::ColourGradient grad(juce::Colours::lime, fillRect.getX(), fillRect.getBottom(),
                                                (level > 0.8f ? juce::Colours::red : CarbonGoldLookAndFeel::goldAccent), fillRect.getX(), fillRect.getY(), false);
                    g.setGradientFill(grad);
                    g.fillRect(fillRect);
                    g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter());
                    g.drawRect(rect, 1.0f);
                };

                drawMeter(lMeter, rmsL);
                drawMeter(rMeter, rmsR);

                // RMS Overlay Text
                char valBuf[64];
                std::snprintf(valBuf, sizeof(valBuf), "L:%.2f R:%.2f", rmsL, rmsR);
                g.setColour(CarbonGoldLookAndFeel::cyberCyan);
                g.setFont(9.0f);
                g.drawText(valBuf, waveArea.reduced(2.0f, 1.0f), juce::Justification::topRight, false);
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::Waveform2D)
            {
                // 2D Waveform Scope (Audio Output or Proper Time Telemetry Plot)
                const auto& scopeBuf = (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                                       ? node->audioScopeBuffer : node->timeScopeBuffer;
                size_t writeIdx = (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                                  ? node->audioScopeWriteIdx : node->timeScopeWriteIdx;

                juce::Path wavePath;
                float midY = scopeBox.getCentreY();
                float h = scopeBox.getHeight() * 0.42f;
                float w = scopeBox.getWidth();

                if (!scopeBuf.empty())
                {
                    size_t len = scopeBuf.size();
                    size_t readStart = (writeIdx + len - 128) % len;

                    if (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                    {
                        // Direct Audio Waveform Oscilloscope
                        for (int i = 0; i < 128; ++i)
                        {
                            float sampleVal = scopeBuf[(readStart + i) % len];
                            float px = scopeBox.getX() + (i / 128.0f) * w;
                            float py = midY - std::clamp(sampleVal, -1.0f, 1.0f) * h;
                            if (i == 0) wavePath.startNewSubPath(px, py);
                            else wavePath.lineTo(px, py);
                        }
                        g.setColour(CarbonGoldLookAndFeel::cyberCyan);
                        g.strokePath(wavePath, juce::PathStrokeType(1.4f));

                        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.85f));
                        g.setFont(9.0f);
                        g.drawText("Audio Wave", scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
                    }
                    else
                    {
                        // Proper Time Telemetry Plot (Relative to baseline / mean)
                        float meanVal = 0.0f;
                        float maxDev = 0.001f;
                        for (int i = 0; i < 128; ++i)
                        {
                            meanVal += scopeBuf[(readStart + i) % len];
                        }
                        meanVal /= 128.0f;

                        for (int i = 0; i < 128; ++i)
                        {
                            maxDev = std::max(maxDev, std::abs(scopeBuf[(readStart + i) % len] - meanVal));
                        }
                        float normScale = (maxDev > 0.0001f) ? (1.0f / maxDev) : 1.0f;

                        for (int i = 0; i < 128; ++i)
                        {
                            float sampleVal = scopeBuf[(readStart + i) % len];
                            float normVal = (sampleVal - meanVal) * std::min(1.0f, normScale * 0.8f);
                            float px = scopeBox.getX() + (i / 128.0f) * w;
                            float py = midY - normVal * h;
                            if (i == 0) wavePath.startNewSubPath(px, py);
                            else wavePath.lineTo(px, py);
                        }
                        g.setColour(CarbonGoldLookAndFeel::royalViolet);
                        g.strokePath(wavePath, juce::PathStrokeType(1.4f));

                        if (node->timeVarMode == RelativisticNode::TimeScopeVariable::MultiTime)
                        {
                            juce::Path tauPath;
                            const auto& tauBuf = node->timeTauScopeBuffer;
                            size_t tauWriteIdx = node->timeTauScopeWriteIdx;
                            size_t tauLen = tauBuf.size();

                            if (tauLen >= 128)
                            {
                                int tauReadStart = (static_cast<int>(tauWriteIdx) + static_cast<int>(tauLen) - 128) % static_cast<int>(tauLen);
                                float tauMean = 0.0f;
                                for (int i = 0; i < 128; ++i) tauMean += tauBuf[(tauReadStart + i) % tauLen];
                                tauMean /= 128.0f;

                                float tauMaxDev = 0.001f;
                                for (int i = 0; i < 128; ++i) tauMaxDev = std::max(tauMaxDev, std::abs(tauBuf[(tauReadStart + i) % tauLen] - tauMean));
                                float tauScale = (tauMaxDev > 0.0001f) ? (1.0f / tauMaxDev) : 1.0f;

                                for (int i = 0; i < 128; ++i)
                                {
                                    float sampleVal = tauBuf[(tauReadStart + i) % tauLen];
                                    float normVal = (sampleVal - tauMean) * std::min(1.0f, tauScale * 0.8f);
                                    float px = scopeBox.getX() + (i / 128.0f) * w;
                                    float py = midY - normVal * h;
                                    if (i == 0) tauPath.startNewSubPath(px, py);
                                    else tauPath.lineTo(px, py);
                                }
                                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                                g.strokePath(tauPath, juce::PathStrokeType(1.4f));
                            }
                        }

                        juce::String valStr = "Speed: " + juce::String(currentGamma, 3) + "x  Offset: " + juce::String(currentTau, 2) + "s";
                        g.setColour(CarbonGoldLookAndFeel::goldAccent);
                        g.setFont(10.0f);
                        g.drawText(valStr, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
                    }
                }
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::ScopeXY)
            {
                // XY Lissajous Scope (Gamma vs Tau)
                g.setColour(CarbonGoldLookAndFeel::royalViolet.withAlpha(0.3f));
                g.drawHorizontalLine(static_cast<int>(scopeBox.getCentreY()), scopeBox.getX(), scopeBox.getRight());
                g.drawVerticalLine(static_cast<int>(scopeBox.getCentreX()), scopeBox.getY(), scopeBox.getBottom());

                juce::Path xyPath;
                float cx = scopeBox.getCentreX();
                float cy = scopeBox.getCentreY();
                float rx = scopeBox.getWidth() * 0.35f;
                float ry = scopeBox.getHeight() * 0.35f;

                for (int pt = 0; pt < 40; ++pt)
                {
                    double phase = (pt / 40.0) * 2.0 * 3.14159;
                    float x = cx + static_cast<float>(std::cos(phase * currentGamma)) * rx;
                    float y = cy + static_cast<float>(std::sin(phase + currentTau * 2.0)) * ry;
                    if (pt == 0) xyPath.startNewSubPath(x, y);
                    else xyPath.lineTo(x, y);
                }
                xyPath.closeSubPath();
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.strokePath(xyPath, juce::PathStrokeType(1.5f));

                juce::String valStr = "Speed: " + juce::String(currentGamma, 3) + "x  Offset: " + juce::String(currentTau, 2) + "s";
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.setFont(10.0f);
                g.drawText(valStr, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::Scope3D)
            {
                // 3D Isometric Time-Space Projection
                float cx = scopeBox.getCentreX();
                float cy = scopeBox.getCentreY();
                float size = std::min(scopeBox.getWidth(), scopeBox.getHeight()) * 0.35f;
                double rotAngle = tHiRes * 0.8;

                auto project3D = [&](float x3d, float y3d, float z3d) -> juce::Point<float> {
                    float rx = x3d * static_cast<float>(std::cos(rotAngle)) - z3d * static_cast<float>(std::sin(rotAngle));
                    float rz = x3d * static_cast<float>(std::sin(rotAngle)) + z3d * static_cast<float>(std::cos(rotAngle));
                    float isoX = cx + (rx - y3d) * 0.707f;
                    float isoY = cy + (rx + y3d) * 0.408f - rz * 0.5f;
                    return { isoX, isoY };
                };

                // 3D Wireframe Cube
                g.setColour(CarbonGoldLookAndFeel::royalViolet.withAlpha(0.5f));
                auto p000 = project3D(-size, -size, -size);
                auto p100 = project3D( size, -size, -size);
                auto p110 = project3D( size,  size, -size);
                auto p010 = project3D(-size,  size, -size);
                auto p001 = project3D(-size, -size,  size);
                auto p101 = project3D( size, -size,  size);
                auto p111 = project3D( size,  size,  size);
                auto p011 = project3D(-size,  size,  size);

                g.drawLine(p000.x, p000.y, p100.x, p100.y, 1.0f);
                g.drawLine(p100.x, p100.y, p110.x, p110.y, 1.0f);
                g.drawLine(p110.x, p110.y, p010.x, p010.y, 1.0f);
                g.drawLine(p010.x, p010.y, p000.x, p000.y, 1.0f);

                g.drawLine(p001.x, p001.y, p101.x, p101.y, 1.0f);
                g.drawLine(p101.x, p101.y, p111.x, p111.y, 1.0f);
                g.drawLine(p111.x, p111.y, p011.x, p011.y, 1.0f);
                g.drawLine(p011.x, p011.y, p001.x, p001.y, 1.0f);

                g.drawLine(p000.x, p000.y, p001.x, p001.y, 1.0f);
                g.drawLine(p100.x, p100.y, p101.x, p101.y, 1.0f);
                g.drawLine(p110.x, p110.y, p111.x, p111.y, 1.0f);
                g.drawLine(p010.x, p010.y, p011.x, p011.y, 1.0f);

                // 3D Relativistic Trajectory Line
                juce::Path path3D;
                for (int i = 0; i < 30; ++i)
                {
                    float tStep = (i / 30.0f) * 2.0f - 1.0f;
                    float xVal = tStep * size;
                    float yVal = static_cast<float>(std::sin(tStep * 3.14159 * currentGamma)) * size;
                    float zVal = static_cast<float>(std::cos(tStep * 3.14159 + currentTau)) * size;
                    auto pt = project3D(xVal, yVal, zVal);
                    if (i == 0) path3D.startNewSubPath(pt);
                    else path3D.lineTo(pt);
                }
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.strokePath(path3D, juce::PathStrokeType(1.5f));

                char valBuf[64];
                std::snprintf(valBuf, sizeof(valBuf), "γ: %.3fx  τ: %+.2fs", currentGamma, currentTau);
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.setFont(10.0f);
                g.drawText(valBuf, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
            }
        }

        if (node->getSymbol() == "patch~" || node->getSymbol() == "osc.patch~")
        {
            g.setColour(CarbonGoldLookAndFeel::royalViolet);
            g.drawText("[🔍 drill-down]", b.removeFromBottom(16.0f), juce::Justification::centred, true);
        }

        // Bottom-Right Corner Resize Grip Handle for Selected Nodes
        if (isSelected)
        {
            auto gripRect = juce::Rectangle<float>(getNodeBounds(*node).getRight() - 12.0f, getNodeBounds(*node).getBottom() - 12.0f, 10.0f, 10.0f);
            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.fillRoundedRectangle(gripRect, 2.0f);
            g.setColour(juce::Colours::black);
            g.drawLine(gripRect.getX() + 2, gripRect.getBottom() - 2, gripRect.getRight() - 2, gripRect.getY() + 2, 1.0f);
        }

        // Inlets
        for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
        {
            auto p = getPortPos(*node, false, i);
            auto type = node->getInlets()[static_cast<size_t>(i)].dataType;
            auto pColor = (type == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                         ((type == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);
            g.setColour(pColor);
            g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }

        // Outlets
        for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
        {
            auto p = getPortPos(*node, true, o);
            auto type = node->getOutlets()[static_cast<size_t>(o)].dataType;
            auto pColor = (type == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                         ((type == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);
            g.setColour(pColor);
            g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }
    }

    // Paint Marquee / Lasso Selection Box
    if (isMarqueeSelecting && !marqueeRect.isEmpty())
    {
        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.15f));
        g.fillRect(marqueeRect);
        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.drawRect(marqueeRect, 1.5f);
    }
}

void RelativisticCanvasComponent::resized()
{
    recenterButton.setBounds(getWidth() - 110, getHeight() - 35, 100, 24);
}

void RelativisticCanvasComponent::spawnObjectEditorAt(juce::Point<float> pos)
{
    lastMousePos = pos;
    isEditingObject = true;
    editingNodeId = -1;
    objectEditor.setText("");
    objectEditor.setBounds(static_cast<int>(pos.x), static_cast<int>(pos.y), 140, 30);
    objectEditor.setVisible(true);
    objectEditor.grabKeyboardFocus();
}

void RelativisticCanvasComponent::spawnObjectEditorForNode(int nodeId)
{
    auto node = getCurrentGraph().getNode(nodeId);
    if (!node) return;

    isEditingObject = true;
    editingNodeId = nodeId;
    lastMousePos = { node->xPos, node->yPos };

    objectEditor.setText(node->getLabel());
    objectEditor.setBounds(static_cast<int>(node->xPos), static_cast<int>(node->yPos), static_cast<int>(std::max(140.0f, static_cast<float>(node->getLabel().length()) * 9.0f + 30.0f)), 30);
    objectEditor.setVisible(true);
    objectEditor.selectAll();
    objectEditor.grabKeyboardFocus();
}

void RelativisticCanvasComponent::commitObjectCreation()
{
    if (!isEditingObject) return;

    juce::String text = objectEditor.getText().trim();
    objectEditor.setVisible(false);
    isEditingObject = false;

    if (text.isNotEmpty())
    {
        if (editingNodeId != -1)
        {
            // Re-editing existing node arguments in-place (PRESERVES ALL WIRE CONNECTIONS!)
            auto oldNode = getCurrentGraph().getNode(editingNodeId);
            if (oldNode)
            {
                oldNode->setLabel(text.toStdString());
                oldNode->receiveMessage(text.toStdString());
            }
        }
        else
        {
            // Spawning new node!
            static int nextTempId = 100;
            auto newNode = RelativisticNodeFactory::createNode(nextTempId++, text.toStdString());
            if (newNode)
            {
                newNode->xPos = lastMousePos.x;
                newNode->yPos = lastMousePos.y;
                getCurrentGraph().addNode(newNode);
            }
        }
    }
    editingNodeId = -1;
    repaint();
}

void RelativisticCanvasComponent::cancelObjectCreation()
{
    objectEditor.setVisible(false);
    isEditingObject = false;
    editingNodeId = -1;
    repaint();
}

void RelativisticCanvasComponent::selectAllNodes()
{
    clearSelection();
    for (const auto& node : getCurrentGraph().getNodes())
    {
        selectNode(node->getId());
    }
    repaint();
}

void RelativisticCanvasComponent::copySelectedNodes()
{
    clipboard.nodes.clear();
    clipboard.connections.clear();

    if (selectedNodeIds.empty()) return;

    for (int id : selectedNodeIds)
    {
        auto node = getCurrentGraph().getNode(id);
        if (!node) continue;

        ClipboardNodeData cNode;
        cNode.originalId = node->getId();
        cNode.symbol = node->getSymbol();
        cNode.label = node->getLabel();
        cNode.xPos = node->xPos;
        cNode.yPos = node->yPos;
        cNode.width = node->width;
        cNode.height = node->height;
        clipboard.nodes.push_back(cNode);
    }

    for (const auto& conn : getCurrentGraph().getConnections())
    {
        if (isNodeSelected(conn.sourceNodeId) && isNodeSelected(conn.destNodeId))
        {
            ClipboardConnectionData cConn;
            cConn.sourceNodeId = conn.sourceNodeId;
            cConn.sourcePortIndex = conn.sourcePortIndex;
            cConn.destNodeId = conn.destNodeId;
            cConn.destPortIndex = conn.destPortIndex;
            cConn.dataType = conn.dataType;
            clipboard.connections.push_back(cConn);
        }
    }
}

void RelativisticCanvasComponent::cutSelectedNodes()
{
    copySelectedNodes();
    deleteSelectedNodes();
}

void RelativisticCanvasComponent::pasteClipboardNodes()
{
    if (clipboard.isEmpty()) return;

    static int nextPasteId = 1000;
    std::unordered_map<int, int> oldToNewIdMap;

    clearSelection();

    // Paste nodes with +30px, +30px offset
    for (const auto& cNode : clipboard.nodes)
    {
        int newId = nextPasteId++;
        oldToNewIdMap[cNode.originalId] = newId;

        auto newNode = RelativisticNodeFactory::createNode(newId, cNode.label);
        if (!newNode) newNode = RelativisticNodeFactory::createNode(newId, cNode.symbol);

        if (newNode)
        {
            newNode->setLabel(cNode.label);
            newNode->xPos = cNode.xPos + 30.0f;
            newNode->yPos = cNode.yPos + 30.0f;
            newNode->width = cNode.width;
            newNode->height = cNode.height;
            getCurrentGraph().addNode(newNode);
            selectNode(newId);
        }
    }

    // Re-create internal connections between pasted nodes
    for (const auto& cConn : clipboard.connections)
    {
        auto srcIt = oldToNewIdMap.find(cConn.sourceNodeId);
        auto destIt = oldToNewIdMap.find(cConn.destNodeId);
        if (srcIt != oldToNewIdMap.end() && destIt != oldToNewIdMap.end())
        {
            getCurrentGraph().addConnection(srcIt->second, cConn.sourcePortIndex, destIt->second, cConn.destPortIndex);
        }
    }

    repaint();
}

void RelativisticCanvasComponent::duplicateSelectedNodes()
{
    copySelectedNodes();
    pasteClipboardNodes();
}

void RelativisticCanvasComponent::deleteSelectedNodes()
{
    if (selectedNodeIds.empty() && selectedConnectionId == -1) return;

    if (selectedConnectionId != -1)
    {
        getCurrentGraph().removeConnection(selectedConnectionId);
        selectedConnectionId = -1;
    }

    for (int id : selectedNodeIds)
    {
        getCurrentGraph().removeNode(id);
    }
    selectedNodeIds.clear();
    repaint();
}

void RelativisticCanvasComponent::spawnMessageBoxForNode(int targetNodeId, const std::string& msgText)
{
    auto targetNode = getCurrentGraph().getNode(targetNodeId);
    if (!targetNode) return;

    static int nextMsgId = 500;
    auto msgNode = RelativisticNodeFactory::createNode(nextMsgId++, "msg " + msgText);
    if (!msgNode) return;

    // Position message box slightly above and to the left of the target node
    msgNode->xPos = std::max(20.0f, targetNode->xPos - 40.0f);
    msgNode->yPos = std::max(40.0f, targetNode->yPos - 60.0f);

    getCurrentGraph().addNode(msgNode);
    // Connect msg node outlet 0 to target node inlet 0
    getCurrentGraph().addConnection(msgNode->getId(), 0, targetNodeId, 0);

    repaint();
}

bool RelativisticCanvasComponent::keyPressed(const juce::KeyPress& key)
{
    bool isCmdOrCtrl = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
    int code = std::tolower(key.getKeyCode());

    // Cmd + A: Select All Nodes
    if (isCmdOrCtrl && code == 'a' && !isEditingObject)
    {
        selectAllNodes();
        return true;
    }

    // Cmd + C: Copy Selected Nodes
    if (isCmdOrCtrl && code == 'c' && !isEditingObject)
    {
        copySelectedNodes();
        return true;
    }

    // Cmd + X: Cut Selected Nodes
    if (isCmdOrCtrl && code == 'x' && !isEditingObject)
    {
        cutSelectedNodes();
        return true;
    }

    // Cmd + V: Paste Clipboard Nodes
    if (isCmdOrCtrl && code == 'v' && !isEditingObject)
    {
        pasteClipboardNodes();
        return true;
    }

    // Cmd + D: Duplicate Selected Nodes
    if (isCmdOrCtrl && code == 'd' && !isEditingObject)
    {
        duplicateSelectedNodes();
        return true;
    }

    // Cmd + 0 or Home key: Recenter Canvas View
    if ((isCmdOrCtrl && key.getKeyCode() == '0') || key.getKeyCode() == juce::KeyPress::homeKey)
    {
        recenterView();
        return true;
    }

    // Cmd + 1: Spawn object box at cursor
    if (isCmdOrCtrl && key.getKeyCode() == '1')
    {
        spawnObjectEditorAt(lastMousePos);
        return true;
    }

    // Enter when blank space selected: Spawn object box at cursor
    if (key.getKeyCode() == juce::KeyPress::returnKey && selectedNodeIds.empty() && !isEditingObject)
    {
        spawnObjectEditorAt(lastMousePos);
        return true;
    }

    // Esc or Delete/Backspace key: Delete selected nodes or selected connection cable
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        if (isEditingObject)
        {
            cancelObjectCreation();
            return true;
        }
        else
        {
            deleteSelectedNodes();
            return true;
        }
    }
    else if ((key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey) && !isEditingObject)
    {
        deleteSelectedNodes();
        return true;
    }

    return false;
}

void RelativisticCanvasComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto pos = e.position;
    lastMousePos = pos;
    auto& currGraph = getCurrentGraph();

    if (isEditingObject && !objectEditor.getBounds().contains(pos.toInt()))
    {
        commitObjectCreation();
    }

    // Check click on Breadcrumb back button
    if (pos.y < 28.0f && graphStack.size() > 1)
    {
        popSubGraphView();
        return;
    }

    // Middle-click, Right-click on blank canvas, or Space+Click pans canvas!
    if (e.mods.isMiddleButtonDown() || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey))
    {
        isPanning = true;
        panStartMouse = pos;
        panStartOffset = { viewOffsetX, viewOffsetY };
        return;
    }

    clearSelection();

    // 1. Check if clicked a Port (Outlet OR Inlet for bi-directional dragging!)
    for (const auto& node : currGraph.getNodes())
    {
        // Check Outlets
        for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
        {
            auto p = getPortPos(*node, true, o);
            if (p.getDistanceFrom(pos) < 12.0f)
            {
                draggingPortNodeId = node->getId();
                draggingPortIdx = o;
                isDraggingFromOutlet = true;
                dragCurrentPos = pos;
                repaint();
                return;
            }
        }

        // Check Inlets
        for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
        {
            auto p = getPortPos(*node, false, i);
            if (p.getDistanceFrom(pos) < 12.0f)
            {
                draggingPortNodeId = node->getId();
                draggingPortIdx = i;
                isDraggingFromOutlet = false;
                dragCurrentPos = pos;
                repaint();
                return;
            }
        }
    }

    // 2. Check if clicked a Node panel
    bool isShift = e.mods.isShiftDown();
    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        if (b.contains(pos))
        {
            if (isShift)
            {
                toggleNodeSelection(node->getId());
            }
            else if (!isNodeSelected(node->getId()))
            {
                clearSelection();
                selectNode(node->getId());
            }

            multiNodeDragStarts.clear();
            for (int id : selectedNodeIds)
            {
                auto n = currGraph.getNode(id);
                if (n) multiNodeDragStarts[id] = { n->xPos, n->yPos };
            }
            nodeDragStartPos = pos;

            if (onNodeSelected) onNodeSelected(node);

            // Check if clicked Realtime Toggle Button [👁]
            auto headerRect = b.withHeight(22.0f);
            auto toggleBtnRect = headerRect.removeFromRight(22.0f).reduced(2.0f);
            if (toggleBtnRect.contains(pos))
            {
                node->toggleRealtimeDisplay();
                repaint();
                return;
            }

            // Check if clicked Scope Mode Button [Audio / Speed / Offset / Flex]
            auto modeBtnRect = headerRect.removeFromRight(46.0f).reduced(2.0f);
            if (modeBtnRect.contains(pos))
            {
                if (node->displayType == RelativisticNode::ScopeDisplayType::TimeFrame)
                {
                    node->cycleTimeVarMode();
                }
                else
                {
                    node->displayType = RelativisticNode::ScopeDisplayType::TimeFrame;
                }
                repaint();
                return;
            }

            // Check if clicked Resize Handle (bottom-right 14x14 px corner)
            auto gripRect = juce::Rectangle<float>(b.getRight() - 14.0f, b.getBottom() - 14.0f, 14.0f, 14.0f);
            if (gripRect.contains(pos))
            {
                isResizingNode = true;
                nodeResizeStartSize = { node->width, node->height };
                repaint();
                return;
            }

            // Double-click on any node opens inline text editor for editing node parameters/text
            if (e.getNumberOfClicks() >= 2)
            {
                spawnObjectEditorForNode(node->getId());
                return;
            }

            // If clicked a MessageNode, dispatch its message down connected cables!
            if (node->getSymbol() == "msg")
            {
                auto msgNode = std::dynamic_pointer_cast<MessageNode>(node);
                if (msgNode)
                {
                    std::string msgText = msgNode->getMessageText();
                    for (const auto& conn : currGraph.getConnections())
                    {
                        if (conn.sourceNodeId == node->getId())
                        {
                            auto dest = currGraph.getNode(conn.destNodeId);
                            if (dest)
                            {
                                dest->receiveMessage(msgText);
                            }
                        }
                    }
                }
            }
            repaint();
            return;
        }
    }

    // 3. Check if clicked near a Patch Connection Cable (Cable Selection)
    for (const auto& conn : currGraph.getConnections())
    {
        auto srcNode = currGraph.getNode(conn.sourceNodeId);
        auto destNode = currGraph.getNode(conn.destNodeId);
        if (!srcNode || !destNode) continue;

        auto p1 = getPortPos(*srcNode, true, conn.sourcePortIndex);
        auto p2 = getPortPos(*destNode, false, conn.destPortIndex);

        // Distance from point to line segment p1-p2
        juce::Line<float> line(p1, p2);
        if (line.findNearestPointTo(pos).getDistanceFrom(pos) < 10.0f)
        {
            clearSelection();
            selectedConnectionId = conn.connectionId;

            // Right-click options on connection cable
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu m;
                m.addItem(1, "Delete Cable Connection");
                m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, connId = conn.connectionId](int res) {
                    if (res == 1) {
                        getCurrentGraph().removeConnection(connId);
                        selectedConnectionId = -1;
                        repaint();
                    }
                });
            }

            repaint();
            return;
        }
    }

    // 4. Clicked blank space: Start marquee lasso selection box
    if (!isShift) clearSelection();
    isMarqueeSelecting = true;
    marqueeStartPos = pos;
    marqueeRect = { pos.x, pos.y, 0.0f, 0.0f };
    repaint();
}

void RelativisticCanvasComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();
    bool clickedNode = false;

    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        if (b.contains(pos))
        {
            clickedNode = true;
            // If composite node, drill down into its internal sub-graph
            if (node->getSymbol() == "patch~" || node->getSymbol() == "osc.patch~")
            {
                auto compNode = std::dynamic_pointer_cast<CompositeNode>(node);
                if (compNode)
                {
                    pushSubGraphView(&compNode->getInternalSubGraph(), compNode->getLabel());
                }
            }
            else
            {
                // Double-click existing node to re-edit its parameters!
                spawnObjectEditorForNode(node->getId());
            }
            break;
        }
    }

    // Double-clicking blank canvas spawns a new object creation box!
    if (!clickedNode)
    {
        spawnObjectEditorAt(pos);
    }
}

void RelativisticCanvasComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();

    if (isPanning)
    {
        viewOffsetX = panStartOffset.x + (pos.x - panStartMouse.x);
        viewOffsetY = panStartOffset.y + (pos.y - panStartMouse.y);
        repaint();
        return;
    }

    if (isMarqueeSelecting)
    {
        marqueeRect = juce::Rectangle<float>::leftTopRightBottom(std::min(marqueeStartPos.x, pos.x), std::min(marqueeStartPos.y, pos.y), std::max(marqueeStartPos.x, pos.x), std::max(marqueeStartPos.y, pos.y));
        if (!e.mods.isShiftDown()) selectedNodeIds.clear();
        for (const auto& node : currGraph.getNodes())
        {
            if (getNodeBounds(*node).intersects(marqueeRect))
            {
                selectNode(node->getId());
            }
        }
        repaint();
        return;
    }

    if (isResizingNode && !selectedNodeIds.empty())
    {
        int targetId = *selectedNodeIds.begin();
        auto node = currGraph.getNode(targetId);
        if (node)
        {
            float newW = std::max(120.0f, nodeResizeStartSize.x + static_cast<float>(e.getDistanceFromDragStartX()));
            float newH = std::max(45.0f, nodeResizeStartSize.y + static_cast<float>(e.getDistanceFromDragStartY()));
            node->width = newW;
            node->height = newH;
            repaint();
            return;
        }
    }

    if (draggingPortNodeId != -1)
    {
        dragCurrentPos = pos;
        repaint();
    }
    else if (!multiNodeDragStarts.empty())
    {
        float dx = static_cast<float>(e.getDistanceFromDragStartX());
        float dy = static_cast<float>(e.getDistanceFromDragStartY());
        for (const auto& kv : multiNodeDragStarts)
        {
            auto node = currGraph.getNode(kv.first);
            if (node)
            {
                node->xPos = kv.second.x + dx;
                node->yPos = kv.second.y + dy;
            }
        }
        repaint();
    }
}

void RelativisticCanvasComponent::mouseUp(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();

    if (isPanning)
    {
        isPanning = false;
        repaint();
        return;
    }

    if (isResizingNode)
    {
        isResizingNode = false;
        repaint();
        return;
    }

    if (isMarqueeSelecting)
    {
        isMarqueeSelecting = false;
        marqueeRect = {};
        repaint();
    }
    multiNodeDragStarts.clear();

    if (draggingPortNodeId != -1)
    {
        for (const auto& node : currGraph.getNodes())
        {
            if (node->getId() == draggingPortNodeId) continue;

            if (isDraggingFromOutlet)
            {
                // Dragged from Outlet -> Drop on Inlet
                for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
                {
                    auto p = getPortPos(*node, false, i);
                    if (p.getDistanceFrom(pos) < 15.0f)
                    {
                        currGraph.addConnection(draggingPortNodeId, draggingPortIdx, node->getId(), i);
                        break;
                    }
                }
            }
            else
            {
                // Dragged from Inlet -> Drop on Outlet
                for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
                {
                    auto p = getPortPos(*node, true, o);
                    if (p.getDistanceFrom(pos) < 15.0f)
                    {
                        currGraph.addConnection(node->getId(), o, draggingPortNodeId, draggingPortIdx);
                        break;
                    }
                }
            }
        }
        draggingPortNodeId = -1;
        draggingPortIdx = -1;
        repaint();
    }
}

void RelativisticCanvasComponent::recenterView()
{
    viewOffsetX = 0.0f;
    viewOffsetY = 0.0f;
    repaint();
}

void RelativisticCanvasComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    viewOffsetX += wheel.deltaX * 120.0f;
    viewOffsetY += wheel.deltaY * 120.0f;
    repaint();
}

} // namespace TimeDilationDAW
