#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SamplePoolComponent.h"

namespace TimeDilationDAW
{

class SamplePoolWindow : public juce::DocumentWindow
{
public:
    SamplePoolWindow(SamplePoolComponent::SpawnNodeCallback spawnCb);
    ~SamplePoolWindow() override;

    void closeButtonPressed() override;

    SamplePoolComponent* getPoolComponent() { return poolComponent.get(); }

private:
    std::unique_ptr<SamplePoolComponent> poolComponent;
};

} // namespace TimeDilationDAW
