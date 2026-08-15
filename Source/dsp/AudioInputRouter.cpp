#include "AudioInputRouter.h"
#include <algorithm>

namespace TimeDilationDAW
{

juce::AudioBuffer<float> AudioInputRouter::globalInputBuffer;
juce::AudioBuffer<float> AudioInputRouter::masterOutputBuffer;
juce::SpinLock AudioInputRouter::bufferLock;

AudioInputRouter& AudioInputRouter::getInstance()
{
    static AudioInputRouter instance;
    return instance;
}

void AudioInputRouter::setGlobalInputBuffer(const juce::AudioBuffer<float>& inBuf)
{
    const juce::SpinLock::ScopedLockType sl(bufferLock);
    globalInputBuffer.makeCopyOf(inBuf, true);
}

const juce::AudioBuffer<float>& AudioInputRouter::getGlobalInputBuffer()
{
    return globalInputBuffer;
}

void AudioInputRouter::setMasterOutputBuffer(const juce::AudioBuffer<float>& outBuf)
{
    const juce::SpinLock::ScopedLockType sl(bufferLock);
    masterOutputBuffer.makeCopyOf(outBuf, true);
}

const juce::AudioBuffer<float>& AudioInputRouter::getMasterOutputBuffer()
{
    return masterOutputBuffer;
}

void AudioInputRouter::fetchAudioBlock(const AudioInputSource& source,
                                      RelativisticNodeGraph& graph,
                                      juce::AudioBuffer<float>& destinationBuffer,
                                      int numSamples)
{
    if (destinationBuffer.getNumSamples() < numSamples)
    {
        destinationBuffer.setSize(std::max(1, destinationBuffer.getNumChannels()), numSamples, false, false, true);
    }
    destinationBuffer.clear();

    const juce::SpinLock::ScopedLockType sl(bufferLock);

    if (source.type == AudioInputType::ExternalMono)
    {
        int chIdx = source.primaryChannel - 1; // 1-indexed to 0-indexed
        if (chIdx >= 0 && chIdx < globalInputBuffer.getNumChannels() && globalInputBuffer.getNumSamples() >= numSamples)
        {
            destinationBuffer.copyFrom(0, 0, globalInputBuffer, chIdx, 0, numSamples);
            if (destinationBuffer.getNumChannels() > 1)
            {
                destinationBuffer.copyFrom(1, 0, globalInputBuffer, chIdx, 0, numSamples);
            }
        }
    }
    else if (source.type == AudioInputType::ExternalStereo)
    {
        int leftCh = source.primaryChannel - 1;
        int rightCh = source.secondaryChannel - 1;

        if (leftCh >= 0 && leftCh < globalInputBuffer.getNumChannels() && globalInputBuffer.getNumSamples() >= numSamples)
        {
            destinationBuffer.copyFrom(0, 0, globalInputBuffer, leftCh, 0, numSamples);
        }
        if (destinationBuffer.getNumChannels() > 1)
        {
            if (rightCh >= 0 && rightCh < globalInputBuffer.getNumChannels() && globalInputBuffer.getNumSamples() >= numSamples)
            {
                destinationBuffer.copyFrom(1, 0, globalInputBuffer, rightCh, 0, numSamples);
            }
            else if (leftCh >= 0 && leftCh < globalInputBuffer.getNumChannels() && globalInputBuffer.getNumSamples() >= numSamples)
            {
                destinationBuffer.copyFrom(1, 0, globalInputBuffer, leftCh, 0, numSamples);
            }
        }
    }
    else if (source.type == AudioInputType::InternalMaster)
    {
        if (masterOutputBuffer.getNumChannels() > 0 && masterOutputBuffer.getNumSamples() >= numSamples)
        {
            destinationBuffer.copyFrom(0, 0, masterOutputBuffer, 0, 0, numSamples);
            if (destinationBuffer.getNumChannels() > 1 && masterOutputBuffer.getNumChannels() > 1)
            {
                destinationBuffer.copyFrom(1, 0, masterOutputBuffer, 1, 0, numSamples);
            }
        }
    }
    else if (source.type == AudioInputType::InternalNodeTap)
    {
        auto node = graph.getNode(source.tapNodeId);
        if (node != nullptr)
        {
            int outletIdx = source.tapOutletIndex;
            if (outletIdx < 0 || outletIdx >= static_cast<int>(node->getOutlets().size()) ||
                node->getOutlets()[static_cast<size_t>(outletIdx)].dataType != PortDataType::Audio)
            {
                for (size_t i = 0; i < node->getOutlets().size(); ++i)
                {
                    if (node->getOutlets()[i].dataType == PortDataType::Audio)
                    {
                        outletIdx = static_cast<int>(i);
                        break;
                    }
                }
            }

            if (outletIdx >= 0 && outletIdx < static_cast<int>(node->getOutlets().size()))
            {
                const auto& nodeBuf = node->getOutletBuffer(outletIdx);
                int copySamples = std::min(numSamples, nodeBuf.getNumSamples());
                if (copySamples > 0)
                {
                    destinationBuffer.copyFrom(0, 0, nodeBuf, 0, 0, copySamples);
                    if (destinationBuffer.getNumChannels() > 1)
                    {
                        if (nodeBuf.getNumChannels() > 1)
                        {
                            destinationBuffer.copyFrom(1, 0, nodeBuf, 1, 0, copySamples);
                        }
                        else
                        {
                            destinationBuffer.copyFrom(1, 0, nodeBuf, 0, 0, copySamples);
                        }
                    }
                }
            }
        }
    }
}

} // namespace TimeDilationDAW
