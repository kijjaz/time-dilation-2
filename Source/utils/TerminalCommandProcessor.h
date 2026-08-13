#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../gui/WorkstationContainerComponent.h"
#include <string>

namespace TimeDilationDAW
{

class TerminalCommandProcessor
{
public:
    static std::string processCommand(WorkstationContainerComponent& workstation, const std::string& commandLine);
    static void runInteractiveLoop(WorkstationContainerComponent& workstation);
    static std::string getHelpText();

private:
    static bool isPortTypeCompatible(PortDataType srcType, PortDataType destType);
    static std::string getPortTypeName(PortDataType type);
};

} // namespace TimeDilationDAW
