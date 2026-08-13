#include "TerminalCommandProcessor.h"
#include "../dsp/RelativisticNodeFactory.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace TimeDilationDAW
{

std::string TerminalCommandProcessor::getPortTypeName(PortDataType type)
{
    switch (type)
    {
        case PortDataType::Audio:   return "Audio (~, Cyan)";
        case PortDataType::Time:    return "Time (time, Royal Violet)";
        case PortDataType::Message: return "Message (msg, Gold)";
        default:                    return "Unknown";
    }
}

bool TerminalCommandProcessor::isPortTypeCompatible(PortDataType srcType, PortDataType destType)
{
    if (srcType == destType) return true;

    // Relativistic Time Dilation drives Audio sampling clock
    if ((srcType == PortDataType::Time && destType == PortDataType::Audio) ||
        (srcType == PortDataType::Audio && destType == PortDataType::Time))
        return true;

    // Float / Numeric Message can set Time dilation factor gamma / offset tau
    if (srcType == PortDataType::Message && destType == PortDataType::Time)
        return true;

    // Direct Audio -> Message or Message -> Audio without converters is disallowed
    return false;
}

std::string TerminalCommandProcessor::getHelpText()
{
    std::ostringstream ss;
    ss << "======================================================================\n";
    ss << "          TIME DILATION DAW 2 — INTERACTIVE TERMINAL CLI HELP        \n";
    ss << "======================================================================\n";
    ss << "Commands:\n";
    ss << "  add <symbol> [args...]       - Create node (e.g. 'add osc~ sin 440', 'add out~')\n";
    ss << "  delete <nodeId>              - Delete node by ID (e.g. 'delete 3')\n";
    ss << "  connect <srcId> <sPort> <destId> <dPort> - Patch cord between nodes\n";
    ss << "  disconnect <connId>          - Remove patch cord by connection ID\n";
    ss << "  send <nodeId> <msg...>       - Dispatch message to node (e.g. 'send 2 freq 880')\n";
    ss << "  bang <nodeId>                - Trigger message box or pluck string\n";
    ss << "  nodes / list                 - Print all nodes, coordinates, and port definitions\n";
    ss << "  cords / connections          - Print all active patch cords\n";
    ss << "  process [blocks]             - Advance DSP audio clock & print RMS telemetry\n";
    ss << "  play / stop                  - Toggle master playback transport\n";
    ss << "  bpm <val>                    - Set project tempo in BPM\n";
    ss << "  save <filepath>              - Save patch to .pdil JSON file\n";
    ss << "  load <filepath>              - Load patch from .pdil JSON file\n";
    ss << "  help                         - Show this help text\n";
    ss << "  quit / exit                  - Exit CLI mode\n";
    ss << "======================================================================\n";
    return ss.str();
}

std::string TerminalCommandProcessor::processCommand(WorkstationContainerComponent& workstation, const std::string& commandLine)
{
    juce::String trimmedStr = juce::String(commandLine).trim();
    if (trimmedStr.isEmpty()) return "";

    auto tokens = juce::StringArray::fromTokens(trimmedStr, " ", "");
    juce::String cmd = tokens[0].toLowerCase();

    auto& graph = workstation.getNodeGraph();

    if (cmd == "help")
    {
        return getHelpText();
    }
    else if (cmd == "add")
    {
        if (tokens.size() < 2)
            return "[ERROR] Usage: add <symbol> [args...] (e.g. 'add osc~ sin 440' or 'add out~')";

        tokens.remove(0); // Remove "add"
        juce::String nodeSpec = tokens.joinIntoString(" ");

        static int nextCliId = 100;
        // Find maximum existing node ID to avoid collisions
        for (const auto& n : graph.getNodes()) nextCliId = std::max(nextCliId, n->getId() + 1);

        int newId = nextCliId++;
        auto newNode = RelativisticNodeFactory::createNode(newId, nodeSpec.toStdString());
        if (!newNode)
        {
            return "[ERROR] Failed to create node with specification: '" + nodeSpec.toStdString() + "'. Check symbol name or parameters.";
        }

        // Layout nodes neatly in a grid
        int col = (graph.getNodes().size()) % 4;
        int row = static_cast<int>(graph.getNodes().size()) / 4;
        newNode->xPos = 80.0f + col * 180.0f;
        newNode->yPos = 80.0f + row * 120.0f;

        graph.addNode(newNode);

        std::ostringstream ss;
        ss << "[SUCCESS] Node #" << newId << " (" << newNode->getSymbol() << ") created successfully! "
           << "Inlets: " << newNode->getInlets().size() << ", Outlets: " << newNode->getOutlets().size() << ". "
           << "Fantastic addition to your relativistic patch!";
        return ss.str();
    }
    else if (cmd == "delete" || cmd == "remove")
    {
        if (tokens.size() < 2) return "[ERROR] Usage: delete <nodeId>";
        int id = tokens[1].getIntValue();
        auto node = graph.getNode(id);
        if (!node) return "[ERROR] Node #" + std::to_string(id) + " not found in graph.";

        std::string sym = node->getSymbol();
        graph.removeNode(id);
        return "[SUCCESS] Node #" + std::to_string(id) + " (" + sym + ") removed cleanly from graph.";
    }
    else if (cmd == "connect" || cmd == "wire" || cmd == "patch")
    {
        if (tokens.size() < 5)
            return "[ERROR] Usage: connect <srcNodeId> <srcPortIdx> <destNodeId> <destPortIdx>";

        int srcId = tokens[1].getIntValue();
        int srcPort = tokens[2].getIntValue();
        int destId = tokens[3].getIntValue();
        int destPort = tokens[4].getIntValue();

        auto srcNode = graph.getNode(srcId);
        auto destNode = graph.getNode(destId);

        if (!srcNode) return "[ERROR] Source Node #" + std::to_string(srcId) + " not found.";
        if (!destNode) return "[ERROR] Destination Node #" + std::to_string(destId) + " not found.";

        if (srcPort < 0 || srcPort >= static_cast<int>(srcNode->getOutlets().size()))
            return "[ERROR] Source Node #" + std::to_string(srcId) + " does not have outlet port #" + std::to_string(srcPort) + ". (Valid range: 0.." + std::to_string(srcNode->getOutlets().size() - 1) + ")";

        if (destPort < 0 || destPort >= static_cast<int>(destNode->getInlets().size()))
            return "[ERROR] Destination Node #" + std::to_string(destId) + " does not have inlet port #" + std::to_string(destPort) + ". (Valid range: 0.." + std::to_string(destNode->getInlets().size() - 1) + ")";

        PortDataType srcType = srcNode->getOutlets()[static_cast<size_t>(srcPort)].dataType;
        PortDataType destType = destNode->getInlets()[static_cast<size_t>(destPort)].dataType;

        if (!isPortTypeCompatible(srcType, destType))
        {
            std::ostringstream ss;
            ss << "[ERROR] Data type mismatch! Cannot patch " << getPortTypeName(srcType)
               << " from Node #" << srcId << " outlet " << srcPort
               << " directly into " << getPortTypeName(destType)
               << " on Node #" << destId << " inlet " << destPort << ".\n";

            if (srcType == PortDataType::Audio && destType == PortDataType::Message)
            {
                ss << "  -> Suggestion: Use a [snapshot~] node to sample audio signal into control messages when banged.";
            }
            else if (srcType == PortDataType::Message && destType == PortDataType::Audio)
            {
                ss << "  -> Suggestion: Use a [sig~] node to convert control numbers into continuous audio signals.";
            }
            return ss.str();
        }

        graph.addConnection(srcId, srcPort, destId, destPort);

        std::ostringstream ss;
        ss << "[SUCCESS] Patch cable connected cleanly!\n"
           << "  Node #" << srcId << " (" << srcNode->getSymbol() << ") Outlet " << srcPort << " [" << getPortTypeName(srcType) << "]\n"
           << "    ===> Node #" << destId << " (" << destNode->getSymbol() << ") Inlet " << destPort << " [" << getPortTypeName(destType) << "]\n"
           << "Brilliant patching! Signal path established.";
        return ss.str();
    }
    else if (cmd == "disconnect")
    {
        if (tokens.size() < 2) return "[ERROR] Usage: disconnect <connId>";
        int connId = tokens[1].getIntValue();
        graph.removeConnection(connId);
        return "[SUCCESS] Connection #" + std::to_string(connId) + " disconnected.";
    }
    else if (cmd == "send" || cmd == "msg")
    {
        if (tokens.size() < 3) return "[ERROR] Usage: send <nodeId> <messageText>";
        int id = tokens[1].getIntValue();
        tokens.remove(0); // remove "send"
        tokens.remove(0); // remove nodeId
        juce::String msgText = tokens.joinIntoString(" ");

        auto node = graph.getNode(id);
        if (!node) return "[ERROR] Node #" + std::to_string(id) + " not found.";

        node->receiveMessage(msgText.toStdString());
        return "[SUCCESS] Dispatched message '" + msgText.toStdString() + "' to Node #" + std::to_string(id) + " (" + node->getSymbol() + ").";
    }
    else if (cmd == "bang")
    {
        if (tokens.size() < 2) return "[ERROR] Usage: bang <nodeId>";
        int id = tokens[1].getIntValue();
        auto node = graph.getNode(id);
        if (!node) return "[ERROR] Node #" + std::to_string(id) + " not found.";

        if (auto mNode = std::dynamic_pointer_cast<MessageNode>(node))
        {
            mNode->triggerMessage();
            return "[SUCCESS] Triggered bang on Message Node #" + std::to_string(id) + " (" + mNode->getMessageText() + ").";
        }
        else if (auto pNode = std::dynamic_pointer_cast<PluckNode>(node))
        {
            pNode->triggerPluck();
            return "[SUCCESS] Triggered pluck string impulse on Node #" + std::to_string(id) + ".";
        }
        else
        {
            node->receiveMessage("bang");
            return "[SUCCESS] Sent 'bang' message to Node #" + std::to_string(id) + " (" + node->getSymbol() + ").";
        }
    }
    else if (cmd == "rms" || cmd == "peak" || cmd == "telemetry")
    {
        if (tokens.size() < 2)
        {
            std::ostringstream ss;
            ss << "--- REAL-TIME AUDIO TELEMETRY (RMS / PEAK) ---\n";
            for (const auto& node : graph.getNodes())
            {
                ss << "  Node #" << node->getId() << " [" << node->getSymbol() << "]: "
                   << "RMS=" << juce::String(node->getRmsLevel(), 4) << "  "
                   << "Peak=" << juce::String(node->getPeakLevel(), 4) << "\n";
            }
            return ss.str();
        }

        int id = tokens[1].getIntValue();
        auto node = graph.getNode(id);
        if (!node) return "[ERROR] Node #" + std::to_string(id) + " not found.";

        std::ostringstream ss;
        ss << "[TELEMETRY] Node #" << id << " (" << node->getSymbol() << "): "
           << "RMS Level = " << juce::String(node->getRmsLevel(), 4) << " | "
           << "Peak Level = " << juce::String(node->getPeakLevel(), 4);
        return ss.str();
    }
    else if (cmd == "nodes" || cmd == "list")
    {
        std::ostringstream ss;
        ss << "--- GRAPH NODES (" << graph.getNodes().size() << " total) ---\n";
        for (const auto& node : graph.getNodes())
        {
            ss << "  Node #" << node->getId() << " [" << node->getSymbol() << "] label=\"" << node->getLabel() << "\" @ ("
               << node->xPos << ", " << node->yPos << ") "
               << "[RMS: " << juce::String(node->getRmsLevel(), 3) << " | Peak: " << juce::String(node->getPeakLevel(), 3) << "]\n";

            ss << "    Inlets (" << node->getInlets().size() << "): ";
            for (size_t i = 0; i < node->getInlets().size(); ++i)
            {
                ss << i << ":" << node->getInlets()[i].name << "[" << getPortTypeName(node->getInlets()[i].dataType) << "] ";
            }
            ss << "\n";

            ss << "    Outlets (" << node->getOutlets().size() << "): ";
            for (size_t o = 0; o < node->getOutlets().size(); ++o)
            {
                ss << o << ":" << node->getOutlets()[o].name << "[" << getPortTypeName(node->getOutlets()[o].dataType) << "] ";
            }
            ss << "\n";
        }
        return ss.str();
    }
    else if (cmd == "cords" || cmd == "connections")
    {
        std::ostringstream ss;
        ss << "--- PATCH CABLE CONNECTIONS (" << graph.getConnections().size() << " total) ---\n";
        int cIdx = 0;
        for (const auto& c : graph.getConnections())
        {
            auto srcNode = graph.getNode(c.sourceNodeId);
            auto destNode = graph.getNode(c.destNodeId);
            std::string srcSym = srcNode ? srcNode->getSymbol() : "?";
            std::string destSym = destNode ? destNode->getSymbol() : "?";

            ss << "  Cable #" << cIdx++ << ": Node #" << c.sourceNodeId << " (" << srcSym << ") Outlet " << c.sourcePortIndex
               << " ===> Node #" << c.destNodeId << " (" << destSym << ") Inlet " << c.destPortIndex
               << " [" << getPortTypeName(c.dataType) << "]\n";
        }
        return ss.str();
    }
    else if (cmd == "process")
    {
        int numBlocks = (tokens.size() >= 2) ? tokens[1].getIntValue() : 1;
        numBlocks = std::clamp(numBlocks, 1, 100);

        workstation.prepareToPlay(512, 96000.0);
        workstation.setIsPlaying(true);

        juce::AudioBuffer<float> masterBuffer(2, 512);
        juce::AudioSourceChannelInfo info(&masterBuffer, 0, 512);

        for (int b = 0; b < numBlocks; ++b)
        {
            masterBuffer.clear();
            workstation.getNextAudioBlock(info);
        }

        std::ostringstream ss;
        ss << "[SUCCESS] Processed " << numBlocks << " audio blocks @ 96kHz.\n";
        ss << "  Master Out Level -> L: " << masterBuffer.getRMSLevel(0, 0, 512)
           << "  R: " << masterBuffer.getRMSLevel(1, 0, 512);
        return ss.str();
    }
    else if (cmd == "play")
    {
        workstation.setIsPlaying(true);
        return "[SUCCESS] Master transport playing. Audio engine active!";
    }
    else if (cmd == "stop")
    {
        workstation.setIsPlaying(false);
        return "[SUCCESS] Master transport stopped. Audio engine paused.";
    }
    else if (cmd == "save")
    {
        if (tokens.size() < 2) return "[ERROR] Usage: save <filepath.pdil>";
        juce::File file(tokens[1]);
        workstation.loadPatchFromFile(file); // sets active file
        workstation.savePatch();
        return "[SUCCESS] Patch saved to '" + file.getFullPathName().toStdString() + "'. Excellent work!";
    }
    else if (cmd == "load")
    {
        if (tokens.size() < 2) return "[ERROR] Usage: load <filepath.pdil>";
        juce::File file(tokens[1]);
        if (!file.existsAsFile()) return "[ERROR] File not found: '" + file.getFullPathName().toStdString() + "'";
        workstation.loadPatchFromFile(file);
        return "[SUCCESS] Patch loaded from '" + file.getFullPathName().toStdString() + "'. Ready to synthesize!";
    }
    else
    {
        return "[ERROR] Unknown command: '" + cmd.toStdString() + "'. Type 'help' for available commands.";
    }
}

void TerminalCommandProcessor::runInteractiveLoop(WorkstationContainerComponent& workstation)
{
    std::cout << getHelpText() << std::endl;
    std::cout << "Starting Time Dilation DAW 2 Interactive CLI session...\n\n";

    std::string line;
    while (true)
    {
        std::cout << "pdil> ";
        if (!std::getline(std::cin, line)) break;

        juce::String trimmed = juce::String(line).trim();
        if (trimmed == "quit" || trimmed == "exit")
        {
            std::cout << "Exiting CLI session. Farewell!\n";
            break;
        }

        std::string result = processCommand(workstation, trimmed.toStdString());
        if (!result.empty())
        {
            std::cout << result << "\n\n";
        }
    }
}

} // namespace TimeDilationDAW
