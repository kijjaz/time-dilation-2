#include "SamplePoolWindow.h"

namespace TimeDilationDAW
{

SamplePoolWindow::SamplePoolWindow(SamplePoolComponent::SpawnNodeCallback spawnCb)
    : DocumentWindow("Audio Sample Pool & Wavetable Manager",
                     juce::Colour(0xff121316),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(false);
    poolComponent = std::make_unique<SamplePoolComponent>();
    if (spawnCb)
    {
        poolComponent->setSpawnNodeCallback(std::move(spawnCb));
    }

    setContentOwned(poolComponent.get(), false);
    poolComponent->setSize(620, 480);

    centreWithSize(620, 480);
    setResizable(true, true);
    setResizeLimits(400, 300, 1200, 900);
}

SamplePoolWindow::~SamplePoolWindow() = default;

void SamplePoolWindow::closeButtonPressed()
{
    setVisible(false);
}

} // namespace TimeDilationDAW
