#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <string>

namespace TimeDilationDAW
{

class AgentTestRunner
{
public:
    static int runHeadlessTest(const juce::StringArray& args);

    static bool testExtremeDilation();
    static bool testFeedbackCycleSCC();
    static bool testCubicInterpolationBounds();
    static bool testCompositeNodeSubGraph();
    static bool testLockFreeMessageRouting();
    static bool testAdaptiveLatencyRampNoClicks();
    static bool testMultiNodeTarjanCycle();
    static bool testGravitationalRedshiftNode();
    static bool testLorentzWarpFilterNode();
    static bool testTachyonGranularNode();
    static bool testJSONSerialization();
    static bool testDynamicAdaptiveLatencyStages();
    static bool testPdControlSuite();
    static bool testAudioSamplePlayback();
    static bool testRelativisticDelayAndPipeSuite();
    static bool testRelativisticTimeSculptingSuite();
    static bool testRelativisticSequencersAndTimelineSuite();
    static bool testTidalCyclesPatternEngine();
    static bool testTidalDynamicSubdivisionDrawer();
    static bool testSamplePoolAndSamplerSuite();
};

} // namespace TimeDilationDAW
