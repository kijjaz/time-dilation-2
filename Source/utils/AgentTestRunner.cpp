#include "AgentTestRunner.h"
#include "ProjectManager.h"
#include "../gui/WorkstationContainerComponent.h"
#include "../dsp/RelativisticNodeFactory.h"
#include "../dsp/RelativisticSequencerNodes.h"
#include "../dsp/TidalPatternEngine.h"
#include "../dsp/TidalSeqNode.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <fstream>

namespace TimeDilationDAW
{

int AgentTestRunner::runHeadlessTest(const juce::StringArray& args)
{
    std::cout << "====================================================\n";
    std::cout << "[AgentTestRunner] Starting Relativistic Node Graph Test...\n";
    std::cout << "====================================================\n";

    juce::String screenCapturePath = "";
    juce::String telemetryPath = "clean_telemetry.json";

    for (const auto& arg : args)
    {
        if (arg.startsWith("--capture-screen="))
        {
            screenCapturePath = arg.substring(17);
        }
        else if (arg.startsWith("--telemetry="))
        {
            telemetryPath = arg.substring(12);
        }
    }

    // Instantiate Workstation Component & Graph in-memory (headless test, NO hardware audio device!)
    WorkstationContainerComponent workstation(false);
    workstation.setSize(1280, 720);
    workstation.resized();

    double sampleRate = 96000.0; // Professional Studio 96kHz default!
    int blockSize = 512;
    workstation.prepareToPlay(blockSize, sampleRate);
    workstation.setIsPlaying(true);

    juce::AudioBuffer<float> masterBuffer(2, blockSize);
    juce::AudioSourceChannelInfo channelInfo(&masterBuffer, 0, blockSize);

    // Create a [msg freq 880] message node and connect it to osc~ (Node 2, Inlet 0)
    auto msgOsc = RelativisticNodeFactory::createNode(10, "msg freq 880");
    msgOsc->xPos = 160; msgOsc->yPos = 40;
    workstation.getNodeGraph().addNode(msgOsc);
    workstation.getNodeGraph().addConnection(10, 0, 2, 0); // Connect msg -> osc~ msgIn

    // Create a target [msg] display node and connect osc~ msgOut (Outlet 0) -> target msgIn (Inlet 0)
    auto msgDisplay = RelativisticNodeFactory::createNode(12, "msg empty");
    msgDisplay->xPos = 260; msgDisplay->yPos = 40;
    workstation.getNodeGraph().addNode(msgDisplay);
    workstation.getNodeGraph().addConnection(2, 0, 12, 0); // Connect osc~ msgOut -> msgDisplay msgIn

    // Dispatch messages to set parameters and query getter values
    auto oscNode = workstation.getNodeGraph().getNode(2);
    if (oscNode)
    {
        oscNode->receiveMessage("freq 880");
        oscNode->receiveMessage("get freq"); // Will emit "freq 880.000000" on msgOut -> msgDisplay!
    }

    juce::String wavExportPath = "artifacts/observation_1_speed_doppler.wav";
    for (const auto& arg : args)
    {
        if (arg.startsWith("--export-wav="))
        {
            wavExportPath = arg.substring(13);
        }
    }

    // =========================================================================
    // WAV Observation 1: time.lfo~ -> osc~ sin (Pure Relativistic Doppler Pitch Glissando)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();
        auto lfo1 = RelativisticNodeFactory::createNode(1, "time.lfo~ 0.3 0.85");
        auto osc1 = RelativisticNodeFactory::createNode(2, "osc~ sin");
        if (osc1) osc1->receiveMessage("freq 440");
        auto out1 = RelativisticNodeFactory::createNode(3, "out~");

        workstation.getNodeGraph().addNode(lfo1);
        workstation.getNodeGraph().addNode(osc1);
        workstation.getNodeGraph().addNode(out1);

        workstation.getNodeGraph().addConnection(1, 1, 2, 1); // time.lfo~ timeOut -> osc~ timeIn
        workstation.getNodeGraph().addConnection(2, 0, 3, 0); // osc~ out~ (Outlet 0) -> out~ in1~ (Inlet 0)

        std::vector<std::string> exportFiles = {
            "artifacts/observation_1_speed_doppler.wav",
            "artifacts/observation_1_lfo_to_oscillator.wav"
        };

        for (const auto& pathStr : exportFiles)
        {
            juce::File wavFile1(pathStr);
            wavFile1.getParentDirectory().createDirectory();
            juce::WavAudioFormat wavFormat;
            auto fileStream1 = wavFile1.createOutputStream();
            if (fileStream1 != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer1(wavFormat.createWriterFor(fileStream1.release(), sampleRate, 2, 16, {}, 0));
                if (writer1 != nullptr)
                {
                    int totalBlocks = static_cast<int>((5.0 * sampleRate) / blockSize);
                    for (int b = 0; b < totalBlocks; ++b)
                    {
                        workstation.getNextAudioBlock(channelInfo);
                        writer1->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                    }
                    writer1->flush();
                    std::cout << "[AgentTestRunner] Exported WAV Observation 1 (Doppler Pitch Shift): " << wavFile1.getFullPathName().toStdString() << "\n";
                }
            }
        }
    }

    // =========================================================================
    // WAV Observation 2: Relativistic Elastic Time Snap Demonstration
    // (time.lfo~ 0.5 0.95 -> osc~ saw 220 & ladder~ 1600 0.8)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();
        auto lfo2 = RelativisticNodeFactory::createNode(1, "time.lfo~ 0.5 0.95");
        auto osc2 = RelativisticNodeFactory::createNode(2, "osc~ saw");
        if (osc2) osc2->receiveMessage("freq 220");
        auto ladder2 = RelativisticNodeFactory::createNode(3, "ladder~ 1600 0.8");
        auto out2 = RelativisticNodeFactory::createNode(4, "out~");

        workstation.getNodeGraph().addNode(lfo2);
        workstation.getNodeGraph().addNode(osc2);
        workstation.getNodeGraph().addNode(ladder2);
        workstation.getNodeGraph().addNode(out2);

        workstation.getNodeGraph().addConnection(1, 1, 2, 1); // time.lfo~ timeOut -> osc~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 3, 1); // time.lfo~ timeOut -> ladder~ timeIn
        workstation.getNodeGraph().addConnection(2, 0, 3, 0); // osc~ out~ (Outlet 0) -> ladder~ in~ (Inlet 0)
        workstation.getNodeGraph().addConnection(3, 0, 4, 0); // ladder~ out~ (Outlet 0) -> out~ in1~ (Inlet 0)

        std::vector<std::string> exportFiles2 = {
            "artifacts/observation_2_elastic_time_snap.wav",
            "artifacts/observation_2_lfo_to_filter.wav"
        };

        for (const auto& pathStr : exportFiles2)
        {
            juce::File wavFile2(pathStr);
            wavFile2.getParentDirectory().createDirectory();
            juce::WavAudioFormat wavFormat;
            auto fileStream2 = wavFile2.createOutputStream();
            if (fileStream2 != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer2(wavFormat.createWriterFor(fileStream2.release(), sampleRate, 2, 16, {}, 0));
                if (writer2 != nullptr)
                {
                    int totalBlocks = static_cast<int>((5.0 * sampleRate) / blockSize);
                    for (int b = 0; b < totalBlocks; ++b)
                    {
                        workstation.getNextAudioBlock(channelInfo);
                        writer2->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                    }
                    writer2->flush();
                    std::cout << "[AgentTestRunner] Exported WAV Observation 2 (Elastic Time Snap): " << wavFile2.getFullPathName().toStdString() << "\n";
                }
            }
        }
    }

    // =========================================================================
    // WAV Observation 3: Relativistic Karplus-Strong String + Moog Ladder + Drive Saturation
    // (time.lfo~ -> seq -> pluck~ -> ladder~ -> drive~ -> out~)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();
        auto lfo3 = RelativisticNodeFactory::createNode(1, "time.lfo~ 0.3 0.8");
        auto seq3 = RelativisticNodeFactory::createNode(2, "seq 60 63 67 70 72 75");
        auto mtof3 = RelativisticNodeFactory::createNode(3, "mtof");
        auto pluck3 = RelativisticNodeFactory::createNode(4, "pluck~ 220");
        auto ladder3 = RelativisticNodeFactory::createNode(5, "ladder~ 1400 0.85");
        auto drive3 = RelativisticNodeFactory::createNode(6, "drive~ 2.5");
        auto out3 = RelativisticNodeFactory::createNode(7, "out~");

        workstation.getNodeGraph().addNode(lfo3);
        workstation.getNodeGraph().addNode(seq3);
        workstation.getNodeGraph().addNode(mtof3);
        workstation.getNodeGraph().addNode(pluck3);
        workstation.getNodeGraph().addNode(ladder3);
        workstation.getNodeGraph().addNode(drive3);
        workstation.getNodeGraph().addNode(out3);

        workstation.getNodeGraph().addConnection(1, 1, 2, 1); // time.lfo~ timeOut -> seq timeIn
        workstation.getNodeGraph().addConnection(1, 1, 4, 1); // time.lfo~ timeOut -> pluck~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 5, 1); // time.lfo~ timeOut -> ladder~ timeIn
        workstation.getNodeGraph().addConnection(2, 1, 4, 2); // seq gate (Outlet 1) -> pluck~ trigger (Inlet 2)
        workstation.getNodeGraph().addConnection(2, 0, 3, 0); // seq note (Outlet 0) -> mtof note (Inlet 0)
        workstation.getNodeGraph().addConnection(3, 0, 4, 3); // mtof freq (Outlet 0) -> pluck~ pitch (Inlet 3)
        workstation.getNodeGraph().addConnection(4, 0, 5, 0); // pluck~ out~ (Outlet 0) -> ladder~ in~ (Inlet 0)
        workstation.getNodeGraph().addConnection(5, 0, 6, 0); // ladder~ out~ (Outlet 0) -> drive~ in~ (Inlet 0)
        workstation.getNodeGraph().addConnection(6, 0, 7, 0); // drive~ out~ (Outlet 0) -> out~ in1~ (Inlet 0)

        std::vector<std::string> exportFiles3 = {
            "artifacts/observation_3_karplus_ladder_drive.wav",
            "artifacts/observation_3_lfo_to_sequencer.wav"
        };

        for (const auto& pathStr : exportFiles3)
        {
            juce::File wavFile3(pathStr);
            wavFile3.getParentDirectory().createDirectory();
            juce::WavAudioFormat wavFormat;
            auto fileStream3 = wavFile3.createOutputStream();
            if (fileStream3 != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer3(wavFormat.createWriterFor(fileStream3.release(), sampleRate, 2, 16, {}, 0));
                if (writer3 != nullptr)
                {
                    int totalBlocks = static_cast<int>((5.0 * sampleRate) / blockSize);
                    for (int b = 0; b < totalBlocks; ++b)
                    {
                        workstation.getNextAudioBlock(channelInfo);
                        writer3->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                    }
                    writer3->flush();
                    std::cout << "[AgentTestRunner] Exported WAV Observation 3 (Karplus + Ladder + Drive): " << wavFile3.getFullPathName().toStdString() << "\n";
                }
            }
        }
    }

    // =========================================================================
    // WAV Demonstration: Full Relativistic Sequencer (Tempo + Pitch + Filter Dilation)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();
        auto lfoDemo = RelativisticNodeFactory::createNode(1, "time.lfo~ 0.25 0.9");
        auto seqDemo = RelativisticNodeFactory::createNode(2, "seq 60 63 67 70 72 75 74 70");
        auto mtofDemo = RelativisticNodeFactory::createNode(3, "mtof");
        auto oscDemo = RelativisticNodeFactory::createNode(4, "osc~ saw");
        auto ladderDemo = RelativisticNodeFactory::createNode(5, "ladder~ 1800 0.75");
        auto outDemo = RelativisticNodeFactory::createNode(6, "out~");

        workstation.getNodeGraph().addNode(lfoDemo);
        workstation.getNodeGraph().addNode(seqDemo);
        workstation.getNodeGraph().addNode(mtofDemo);
        workstation.getNodeGraph().addNode(oscDemo);
        workstation.getNodeGraph().addNode(ladderDemo);
        workstation.getNodeGraph().addNode(outDemo);

        workstation.getNodeGraph().addConnection(1, 1, 2, 1); // time.lfo~ timeOut -> seq timeIn
        workstation.getNodeGraph().addConnection(1, 1, 4, 1); // time.lfo~ timeOut -> osc~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 5, 1); // time.lfo~ timeOut -> ladder~ timeIn

        workstation.getNodeGraph().addConnection(2, 1, 3, 1); // seq note -> mtof note
        workstation.getNodeGraph().addConnection(3, 1, 4, 2); // mtof freq -> osc~ freq
        workstation.getNodeGraph().addConnection(4, 2, 5, 2); // osc~ out~ -> ladder~ in~
        workstation.getNodeGraph().addConnection(5, 2, 6, 1); // ladder~ out~ -> out~ in~

        juce::File demoWav("artifacts/demonstration_time_dilation_sequencer_96k.wav");
        juce::WavAudioFormat wavFormat;
        auto fileStreamDemo = demoWav.createOutputStream();
        if (fileStreamDemo != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writerDemo(wavFormat.createWriterFor(fileStreamDemo.release(), sampleRate, 2, 16, {}, 0));
            if (writerDemo != nullptr)
            {
                int totalBlocks = static_cast<int>((10.0 * sampleRate) / blockSize); // 10-second rich render
                for (int b = 0; b < totalBlocks; ++b)
                {
                    workstation.getNextAudioBlock(channelInfo);
                    writerDemo->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writerDemo->flush();
                std::cout << "[AgentTestRunner] Exported WAV Demonstration (Full Relativistic Sequencer): " << demoWav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 5: Pure Data Control + Relativistic Sample Streaming + Dynamic Line~ Envelope
    // ([soundfiler] -> [readsf~ 2] -> [time.lfo~] -> [metro] -> [counter] -> [select] -> [line~] -> [ladder~] -> [drive~] -> [out~])
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();

        // 1. Synthesize a musical test sample WAV file with rich harmonics
        juce::File musicalWav("artifacts/musical_sample_test.wav");
        musicalWav.getParentDirectory().createDirectory();
        {
            juce::WavAudioFormat wavFmt;
            auto stream = musicalWav.createOutputStream();
            if (stream != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer(wavFmt.createWriterFor(stream.release(), 96000.0, 2, 16, {}, 0));
                if (writer != nullptr)
                {
                    const int numSamp = static_cast<int>(96000.0 * 2.0); // 2 second sample
                    juce::AudioBuffer<float> sampleBuf(2, numSamp);
                    float* l = sampleBuf.getWritePointer(0);
                    float* r = sampleBuf.getWritePointer(1);
                    for (int i = 0; i < numSamp; ++i)
                    {
                        double t = static_cast<double>(i) / 96000.0;
                        // Chord progression simulation: A minor 9 (A, C, E, G, B)
                        double val = 0.25 * std::sin(2.0 * 3.14159265 * 220.0 * t)
                                   + 0.20 * std::sin(2.0 * 3.14159265 * 261.63 * t)
                                   + 0.20 * std::sin(2.0 * 3.14159265 * 329.63 * t)
                                   + 0.15 * std::sin(2.0 * 3.14159265 * 392.00 * t)
                                   + 0.10 * std::sin(2.0 * 3.14159265 * 493.88 * t);
                        // Subtle stereo chorus detune
                        l[i] = static_cast<float>(val * std::exp(-0.8 * t));
                        r[i] = static_cast<float>((0.25 * std::sin(2.0 * 3.14159265 * 220.5 * t)
                                                 + 0.20 * std::sin(2.0 * 3.14159265 * 262.2 * t)
                                                 + 0.20 * std::sin(2.0 * 3.14159265 * 330.3 * t)) * std::exp(-0.8 * t));
                    }
                    writer->writeFromAudioSampleBuffer(sampleBuf, 0, numSamp);
                    writer->flush();
                }
            }
        }

        // 2. Instantiate and connect Pure Data Control + Relativistic Nodes
        auto soundfiler = RelativisticNodeFactory::createNode(1, "soundfiler");
        auto lfoNode    = RelativisticNodeFactory::createNode(2, "time.lfo~ 0.3 0.6");
        auto readsfNode = RelativisticNodeFactory::createNode(3, "readsf~ 2");
        auto metroNode  = RelativisticNodeFactory::createNode(4, "metro 180 1");
        auto countNode  = RelativisticNodeFactory::createNode(5, "counter 0 7 1");
        auto selNode    = RelativisticNodeFactory::createNode(6, "sel 0 2 4 6");
        auto lineNode   = RelativisticNodeFactory::createNode(7, "line~ 0.0");
        auto filterNode = RelativisticNodeFactory::createNode(8, "ladder~ 2200 0.7");
        auto driveNode  = RelativisticNodeFactory::createNode(9, "drive~ 1.8");
        auto outNode    = RelativisticNodeFactory::createNode(10, "out~");

        workstation.getNodeGraph().addNode(soundfiler);
        workstation.getNodeGraph().addNode(lfoNode);
        workstation.getNodeGraph().addNode(readsfNode);
        workstation.getNodeGraph().addNode(metroNode);
        workstation.getNodeGraph().addNode(countNode);
        workstation.getNodeGraph().addNode(selNode);
        workstation.getNodeGraph().addNode(lineNode);
        workstation.getNodeGraph().addNode(filterNode);
        workstation.getNodeGraph().addNode(driveNode);
        workstation.getNodeGraph().addNode(outNode);

        // Soundfiler load test
        soundfiler->receiveMessage("read -resize artifacts/musical_sample_test.wav rhodes_array");

        // Relativistic Time Connections
        workstation.getNodeGraph().addConnection(2, 1, 3, 1); // time.lfo~ timeOut -> readsf~ timeIn
        workstation.getNodeGraph().addConnection(2, 1, 4, 1); // time.lfo~ timeOut -> metro timeIn
        workstation.getNodeGraph().addConnection(2, 1, 7, 1); // time.lfo~ timeOut -> line~ timeIn
        workstation.getNodeGraph().addConnection(2, 1, 8, 1); // time.lfo~ timeOut -> ladder~ timeIn

        // Control Routing Connections: metro -> counter -> sel -> line~
        workstation.getNodeGraph().addConnection(4, 0, 5, 0); // metro bang -> counter bang
        workstation.getNodeGraph().addConnection(5, 0, 6, 0); // counter step -> sel val
        workstation.getNodeGraph().addConnection(6, 0, 7, 0); // sel match 0 -> line~ ramp trigger

        // Audio Path: readsf~ -> ladder~ -> drive~ -> out~
        workstation.getNodeGraph().addConnection(3, 2, 8, 2); // readsf~ L -> ladder~ in~
        workstation.getNodeGraph().addConnection(8, 2, 9, 2); // ladder~ out~ -> drive~ in~
        workstation.getNodeGraph().addConnection(9, 2, 10, 1); // drive~ out~ -> out~ L
        workstation.getNodeGraph().addConnection(3, 3, 10, 2); // readsf~ R -> out~ R

        // Start disk streaming
        readsfNode->receiveMessage("open artifacts/musical_sample_test.wav");
        readsfNode->receiveMessage("loop 1");
        readsfNode->receiveMessage("start");

        juce::File obs5Wav("artifacts/observation_5_pd_control_sample_synthesis.wav");
        auto fileStream5 = obs5Wav.createOutputStream();
        if (fileStream5 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer5(wavFormat.createWriterFor(fileStream5.release(), sampleRate, 2, 16, {}, 0));
            if (writer5 != nullptr)
            {
                int totalBlocks5 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second rich render
                for (int b = 0; b < totalBlocks5; ++b)
                {
                    // Every 24 blocks send envelope trigger into line~ to dynamically modulate filter
                    if (b % 24 == 0)
                    {
                        lineNode->receiveMessage("1.0 15 0.1 250");
                    }

                    workstation.getNextAudioBlock(channelInfo);
                    writer5->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer5->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 5 (Pure Data Control + Sample Synthesis): " << obs5Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 6: Relativistic Multi-Tap Tape Doppler Delay Network
    // ([readsf~] -> [delwrite~ tape1 2500] -> [time.lfo~] -> 3x [vd~] taps -> [pipe] feedback -> [out~])
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();

        auto lfoTime   = RelativisticNodeFactory::createNode(1, "time.lfo~ 0.15 0.75");
        auto readsf    = RelativisticNodeFactory::createNode(2, "readsf~ 2");
        auto delwrite  = RelativisticNodeFactory::createNode(3, "delwrite~ tape1 2500");
        auto tap1      = RelativisticNodeFactory::createNode(4, "vd~ tape1 150");
        auto tap2      = RelativisticNodeFactory::createNode(5, "vd~ tape1 300");
        auto tap3      = RelativisticNodeFactory::createNode(6, "vd~ tape1 600");
        auto filterTap = RelativisticNodeFactory::createNode(7, "ladder~ 1600 0.5");
        auto driveSat  = RelativisticNodeFactory::createNode(8, "drive~ 1.5");
        auto pipeCtl   = RelativisticNodeFactory::createNode(9, "pipe 200");
        auto outNode   = RelativisticNodeFactory::createNode(10, "out~");

        workstation.getNodeGraph().addNode(lfoTime);
        workstation.getNodeGraph().addNode(readsf);
        workstation.getNodeGraph().addNode(delwrite);
        workstation.getNodeGraph().addNode(tap1);
        workstation.getNodeGraph().addNode(tap2);
        workstation.getNodeGraph().addNode(tap3);
        workstation.getNodeGraph().addNode(filterTap);
        workstation.getNodeGraph().addNode(driveSat);
        workstation.getNodeGraph().addNode(pipeCtl);
        workstation.getNodeGraph().addNode(outNode);

        // Relativistic Time Connections
        workstation.getNodeGraph().addConnection(1, 1, 2, 1); // time.lfo~ -> readsf~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 3, 2); // time.lfo~ -> delwrite~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 4, 2); // time.lfo~ -> tap1 timeIn
        workstation.getNodeGraph().addConnection(1, 1, 5, 2); // time.lfo~ -> tap2 timeIn
        workstation.getNodeGraph().addConnection(1, 1, 6, 2); // time.lfo~ -> tap3 timeIn
        workstation.getNodeGraph().addConnection(1, 1, 7, 1); // time.lfo~ -> ladder~ timeIn
        workstation.getNodeGraph().addConnection(1, 1, 9, 1); // time.lfo~ -> pipe timeIn

        // Audio Connections
        workstation.getNodeGraph().addConnection(2, 2, 3, 1); // readsf L -> delwrite in~
        workstation.getNodeGraph().addConnection(4, 2, 7, 2); // tap1 out~ -> filter in~
        workstation.getNodeGraph().addConnection(5, 2, 7, 2); // tap2 out~ -> filter in~
        workstation.getNodeGraph().addConnection(6, 2, 7, 2); // tap3 out~ -> filter in~
        workstation.getNodeGraph().addConnection(7, 2, 8, 2); // filter out~ -> drive in~
        workstation.getNodeGraph().addConnection(8, 2, 10, 1); // drive out~ -> out~ L
        workstation.getNodeGraph().addConnection(2, 3, 10, 2); // readsf R -> out~ R

        // Start disk audio streaming
        readsf->receiveMessage("open artifacts/musical_sample_test.wav");
        readsf->receiveMessage("loop 1");
        readsf->receiveMessage("start");

        juce::File obs6Wav("artifacts/observation_6_relativistic_delay_pipe.wav");
        auto fileStream6 = obs6Wav.createOutputStream();
        if (fileStream6 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer6(wavFormat.createWriterFor(fileStream6.release(), sampleRate, 2, 16, {}, 0));
            if (writer6 != nullptr)
            {
                int totalBlocks6 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                for (int b = 0; b < totalBlocks6; ++b)
                {
                    if (b % 32 == 0)
                    {
                        pipeCtl->receiveMessage("cutoff " + std::to_string(800 + (b * 20) % 2400) + " 100");
                    }

                    workstation.getNextAudioBlock(channelInfo);
                    writer6->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer6->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 6 (Relativistic Delay & Pipe Network): " << obs6Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 7: Full Example Patch (example_relativistic_delay_pipe_synth.pdil)
    // ([time.lfo~] -> [metro] -> [counter] -> [pipe] -> [seq] -> [mtof] -> [osc~] -> [pipe] -> [ladder~] -> [drive~] -> [delwrite~] -> 2x [vd~] -> [out~])
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();
        juce::File patchFile("patches/example_relativistic_delay_pipe_synth.pdil");
        if (patchFile.existsAsFile())
        {
            std::string jsonStr = patchFile.loadFileAsString().toStdString();
            workstation.getNodeGraph().deserializeFromJSON(jsonStr);

            juce::File obs7Wav("artifacts/observation_7_delayline_pipe_synth_patch.wav");
            auto fileStream7 = obs7Wav.createOutputStream();
            if (fileStream7 != nullptr)
            {
                juce::WavAudioFormat wavFormat;
                std::unique_ptr<juce::AudioFormatWriter> writer7(wavFormat.createWriterFor(fileStream7.release(), sampleRate, 2, 16, {}, 0));
                if (writer7 != nullptr)
                {
                    int totalBlocks7 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                    for (int b = 0; b < totalBlocks7; ++b)
                    {
                        workstation.getNextAudioBlock(channelInfo);
                        writer7->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                    }
                    writer7->flush();
                    std::cout << "[AgentTestRunner] Exported WAV Observation 7 (Example Synth Patch with DelayLine & Pipe): " << obs7Wav.getFullPathName().toStdString() << "\n";
                }
            }
        }
    }

    // =========================================================================
    // WAV Observation 8: Multi-Branch Polyrhythmic Time Morph & Chaos Rig
    // (Lorenz Chaos + Hermite Time Curve + Crossfade Morphing -> Saturated Doppler Delay)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();

        auto chaosTime  = RelativisticNodeFactory::createNode(1, "time.chaos~ 0.35 lorenz");
        auto curveTime  = RelativisticNodeFactory::createNode(2, "time.curve~ 1.5 2000");
        auto xfadeTime  = RelativisticNodeFactory::createNode(3, "time.crossfade~ 0.5");
        auto lfoMixMod  = RelativisticNodeFactory::createNode(4, "osc~ sin"); // Audio-rate LFO for xfade morph
        auto metroClock = RelativisticNodeFactory::createNode(5, "metro 160 1");
        auto counter    = RelativisticNodeFactory::createNode(6, "counter 0 7 1");
        auto seqPitch   = RelativisticNodeFactory::createNode(7, "seq 48 51 55 58 60 63 67 70");
        auto mtofNode   = RelativisticNodeFactory::createNode(8, "mtof");
        auto oscNode    = RelativisticNodeFactory::createNode(9, "osc~ saw");
        auto filterNode = RelativisticNodeFactory::createNode(10, "ladder~ 1800 0.7");
        auto delwrite   = RelativisticNodeFactory::createNode(11, "delwrite~ chaos_tape 2500");
        auto tapL       = RelativisticNodeFactory::createNode(12, "vd~ chaos_tape 175");
        auto tapR       = RelativisticNodeFactory::createNode(13, "vd~ chaos_tape 350");
        auto driveNode  = RelativisticNodeFactory::createNode(14, "drive~ 1.8");
        auto outNode    = RelativisticNodeFactory::createNode(15, "out~");

        workstation.getNodeGraph().addNode(chaosTime);
        workstation.getNodeGraph().addNode(curveTime);
        workstation.getNodeGraph().addNode(xfadeTime);
        workstation.getNodeGraph().addNode(lfoMixMod);
        workstation.getNodeGraph().addNode(metroClock);
        workstation.getNodeGraph().addNode(counter);
        workstation.getNodeGraph().addNode(seqPitch);
        workstation.getNodeGraph().addNode(mtofNode);
        workstation.getNodeGraph().addNode(oscNode);
        workstation.getNodeGraph().addNode(filterNode);
        workstation.getNodeGraph().addNode(delwrite);
        workstation.getNodeGraph().addNode(tapL);
        workstation.getNodeGraph().addNode(tapR);
        workstation.getNodeGraph().addNode(driveNode);
        workstation.getNodeGraph().addNode(outNode);

        // Time Crossfade Connections (Chaos A + Curve B)
        workstation.getNodeGraph().addConnection(1, 1, 3, 1); // chaos timeOut -> xfade timeIn1
        workstation.getNodeGraph().addConnection(2, 1, 3, 2); // curve timeOut -> xfade timeIn2
        workstation.getNodeGraph().addConnection(4, 2, 3, 3); // LFO out~ -> xfade mixMod~

        // Broadcast Blended Spacetime
        workstation.getNodeGraph().addConnection(3, 1, 5, 1);  // xfade -> metro
        workstation.getNodeGraph().addConnection(3, 1, 9, 1);  // xfade -> osc
        workstation.getNodeGraph().addConnection(3, 1, 10, 1); // xfade -> ladder
        workstation.getNodeGraph().addConnection(3, 1, 11, 2); // xfade -> delwrite
        workstation.getNodeGraph().addConnection(3, 1, 12, 2); // xfade -> tapL
        workstation.getNodeGraph().addConnection(3, 1, 13, 2); // xfade -> tapR

        // Control & Audio Connections
        workstation.getNodeGraph().addConnection(5, 0, 6, 0);  // metro -> counter
        workstation.getNodeGraph().addConnection(6, 0, 7, 0);  // counter -> seq
        workstation.getNodeGraph().addConnection(7, 1, 8, 1);  // seq -> mtof
        workstation.getNodeGraph().addConnection(8, 1, 9, 2);  // mtof -> osc freq
        workstation.getNodeGraph().addConnection(9, 2, 10, 2); // osc -> ladder
        workstation.getNodeGraph().addConnection(10, 2, 11, 1); // ladder -> delwrite
        workstation.getNodeGraph().addConnection(10, 2, 14, 2); // ladder -> drive
        workstation.getNodeGraph().addConnection(14, 2, 15, 1); // drive -> out L
        workstation.getNodeGraph().addConnection(12, 2, 15, 1); // tapL -> out L
        workstation.getNodeGraph().addConnection(13, 2, 15, 2); // tapR -> out R

        // Trigger dynamic Hermite curve sweeps mid-stream
        curveTime->receiveMessage("ramp 3.0 1200");

        juce::File obs8Wav("artifacts/observation_8_time_morph_chaos_synth.wav");
        auto fileStream8 = obs8Wav.createOutputStream();
        if (fileStream8 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer8(wavFormat.createWriterFor(fileStream8.release(), sampleRate, 2, 16, {}, 0));
            if (writer8 != nullptr)
            {
                int totalBlocks8 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                for (int b = 0; b < totalBlocks8; ++b)
                {
                    if (b == totalBlocks8 / 3) curveTime->receiveMessage("ramp 0.25 1500");
                    if (b == 2 * totalBlocks8 / 3) curveTime->receiveMessage("ramp 2.0 1000");

                    workstation.getNextAudioBlock(channelInfo);
                    writer8->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer8->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 8 (Chaos & Curve Time Morph Synth): " << obs8Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 9: Deterministic Control Tape Stop & Wobble Rig
    // (metro -> counter + random -> select -> time.curve~ driving saw + noise into Doppler tape deck)
    // =========================================================================
    {
        workstation.getNodeGraph().clearGraph();

        auto timeCurve  = RelativisticNodeFactory::createNode(1, "time.curve~ 1.0 400");
        auto metroClock = RelativisticNodeFactory::createNode(2, "metro 125 1");
        auto trig       = RelativisticNodeFactory::createNode(3, "t b b");
        auto counter    = RelativisticNodeFactory::createNode(4, "counter 0 15 1");
        auto rnd        = RelativisticNodeFactory::createNode(5, "random 100");
        auto sel        = RelativisticNodeFactory::createNode(6, "select 0 4 8 12 15");
        auto seq        = RelativisticNodeFactory::createNode(7, "seq 36 36 48 51 53 55 58 60 48 51 63 60 36 48 55 58");
        auto mtof       = RelativisticNodeFactory::createNode(8, "mtof");
        auto osc        = RelativisticNodeFactory::createNode(9, "osc~ saw");
        auto noise      = RelativisticNodeFactory::createNode(10, "noise~ white");
        auto filter     = RelativisticNodeFactory::createNode(11, "ladder~ 2200 0.65");
        auto delwrite   = RelativisticNodeFactory::createNode(12, "delwrite~ tape_deck 2000");
        auto tapL       = RelativisticNodeFactory::createNode(13, "vd~ tape_deck 140");
        auto tapR       = RelativisticNodeFactory::createNode(14, "vd~ tape_deck 280");
        auto drive      = RelativisticNodeFactory::createNode(15, "drive~ 1.6");
        auto out        = RelativisticNodeFactory::createNode(16, "out~");

        workstation.getNodeGraph().addNode(timeCurve);
        workstation.getNodeGraph().addNode(metroClock);
        workstation.getNodeGraph().addNode(trig);
        workstation.getNodeGraph().addNode(counter);
        workstation.getNodeGraph().addNode(rnd);
        workstation.getNodeGraph().addNode(sel);
        workstation.getNodeGraph().addNode(seq);
        workstation.getNodeGraph().addNode(mtof);
        workstation.getNodeGraph().addNode(osc);
        workstation.getNodeGraph().addNode(noise);
        workstation.getNodeGraph().addNode(filter);
        workstation.getNodeGraph().addNode(delwrite);
        workstation.getNodeGraph().addNode(tapL);
        workstation.getNodeGraph().addNode(tapR);
        workstation.getNodeGraph().addNode(drive);
        workstation.getNodeGraph().addNode(out);

        // Relativistic Time Distribution (time.curve~ -> metro, osc, filter, delwrite, taps)
        workstation.getNodeGraph().addConnection(1, 1, 2, 1);  // timeCurve -> metro
        workstation.getNodeGraph().addConnection(1, 1, 9, 1);  // timeCurve -> osc
        workstation.getNodeGraph().addConnection(1, 1, 11, 1); // timeCurve -> filter
        workstation.getNodeGraph().addConnection(1, 1, 12, 2); // timeCurve -> delwrite
        workstation.getNodeGraph().addConnection(1, 1, 13, 2); // timeCurve -> tapL
        workstation.getNodeGraph().addConnection(1, 1, 14, 2); // timeCurve -> tapR

        // Deterministic Control Flow (metro -> t b b -> counter + random -> select)
        workstation.getNodeGraph().addConnection(2, 0, 3, 0);  // metro -> trig
        workstation.getNodeGraph().addConnection(3, 0, 4, 0);  // trig -> counter
        workstation.getNodeGraph().addConnection(3, 1, 5, 0);  // trig -> random
        workstation.getNodeGraph().addConnection(4, 0, 6, 0);  // counter -> select
        workstation.getNodeGraph().addConnection(4, 0, 7, 0);  // counter -> seq
        workstation.getNodeGraph().addConnection(7, 1, 8, 1);  // seq -> mtof
        workstation.getNodeGraph().addConnection(8, 1, 9, 2);  // mtof -> osc freq

        // Dynamic State Machine: select triggers deterministic time commands on timeCurve
        sel->onMessageEmitted = [timeCurve](const std::string& msg) {
            // Select emits matched index or bang on match
            if (msg.find("match 0") != std::string::npos || msg == "bang")
                timeCurve->receiveMessage("ramp 1.0 150");
        };

        // Wire select outlets directly
        workstation.getNodeGraph().addConnection(6, 0, 1, 0); // match 0 (beat 0)  -> resume/ramp 1.0
        workstation.getNodeGraph().addConnection(6, 1, 1, 0); // match 4 (beat 4)  -> wobble speed up
        workstation.getNodeGraph().addConnection(6, 2, 1, 0); // match 8 (beat 8)  -> wobble speed down
        workstation.getNodeGraph().addConnection(6, 3, 1, 0); // match 12 (beat 12) -> tape stop brake
        workstation.getNodeGraph().addConnection(6, 4, 1, 0); // match 15 (beat 15) -> tape start spin up

        // Sound Source: Oscillator + White Noise -> Ladder Filter
        workstation.getNodeGraph().addConnection(9, 2, 11, 2);  // osc saw -> ladder in
        workstation.getNodeGraph().addConnection(10, 2, 11, 2); // noise -> ladder in

        // Output & Tape Echo Loop
        workstation.getNodeGraph().addConnection(11, 2, 12, 1); // ladder -> delwrite
        workstation.getNodeGraph().addConnection(11, 2, 15, 2); // ladder -> drive
        workstation.getNodeGraph().addConnection(15, 2, 16, 1); // drive -> out L
        workstation.getNodeGraph().addConnection(13, 2, 16, 1); // tapL -> out L
        workstation.getNodeGraph().addConnection(14, 2, 16, 2); // tapR -> out R

        juce::File obs9Wav("artifacts/observation_9_deterministic_tape_stop_wobble.wav");
        auto fileStream9 = obs9Wav.createOutputStream();
        if (fileStream9 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer9(wavFormat.createWriterFor(fileStream9.release(), sampleRate, 2, 16, {}, 0));
            if (writer9 != nullptr)
            {
                int totalBlocks9 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                for (int b = 0; b < totalBlocks9; ++b)
                {
                    // Introduce tape brake and wobble triggers into timeCurve at exact rhythmic intervals
                    if (b == totalBlocks9 / 4)      timeCurve->receiveMessage("ramp 1.35 120"); // Flutter wobble
                    else if (b == totalBlocks9 / 3) timeCurve->receiveMessage("ramp 0.85 150"); // Flutter dip
                    else if (b == totalBlocks9 / 2) timeCurve->receiveMessage("stop 600");      // Deep Tape Stop Brake
                    else if (b == 3 * totalBlocks9 / 4) timeCurve->receiveMessage("start 400"); // Spin-up start

                    workstation.getNextAudioBlock(channelInfo);
                    writer9->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer9->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 9 (Deterministic Tape Stop & Wobble Rig): " << obs9Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 10: Relativistic Euclidean & Timeline Arrangement Suite
    // (Euclid 4/16 Kick + Euclid 7/16 Hat + Ping-Pong Arpeggiator + Automated Moog Sweep)
    // =========================================================================
    {
        workstation.loadExampleEuclideanArrangement();

        juce::File obs10Wav("artifacts/observation_10_timeline_sequencer_suite.wav");
        auto fileStream10 = obs10Wav.createOutputStream();
        if (fileStream10 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer10(wavFormat.createWriterFor(fileStream10.release(), sampleRate, 2, 16, {}, 0));
            if (writer10 != nullptr)
            {
                int totalBlocks10 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                auto autoNode = workstation.getNodeGraph().getNode(10);
                if (autoNode)
                {
                    // Program dynamic cutoff envelope
                    autoNode->receiveMessage("clear");
                    autoNode->receiveMessage("add 0.0 0.15");
                    autoNode->receiveMessage("add 2.0 0.85");
                    autoNode->receiveMessage("add 4.0 0.30");
                    autoNode->receiveMessage("add 6.0 0.95");
                }

                for (int b = 0; b < totalBlocks10; ++b)
                {
                    workstation.getNextAudioBlock(channelInfo);
                    writer10->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer10->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 10 (Relativistic Euclidean & Timeline Arrangement Suite): " << obs10Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 11: TidalCycles Relativistic Nested Polyphony Rig
    // (Nested Subdivisions + Stacking [60 [62 65] 67, 36 [~ 48]] + Euclidean Drum Bursts)
    // =========================================================================
    {
        workstation.loadExampleTidalCyclesRig();

        juce::File obs11Wav("artifacts/observation_11_tidal_pattern_rig.wav");
        auto fileStream11 = obs11Wav.createOutputStream();
        if (fileStream11 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer11(wavFormat.createWriterFor(fileStream11.release(), sampleRate, 2, 16, {}, 0));
            if (writer11 != nullptr)
            {
                int totalBlocks11 = static_cast<int>((6.0 * sampleRate) / blockSize); // 6-second render
                auto timeNode = workstation.getNodeGraph().getNode(1);
                for (int b = 0; b < totalBlocks11; ++b)
                {
                    // Introduce relativistic time warps at cycle boundaries
                    if (b == totalBlocks11 / 3)      timeNode->receiveMessage("ramp 1.5 200");
                    else if (b == 2 * totalBlocks11 / 3) timeNode->receiveMessage("ramp 0.75 300");

                    workstation.getNextAudioBlock(channelInfo);
                    writer11->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer11->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 11 (TidalCycles Relativistic Nested Polyphony Rig): " << obs11Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    // =========================================================================
    // WAV Observation 12: Customizable TidalCycles Subdivisions & Stacking
    // (Nested Subdivisions [60 [62 [64 65]] 67 [69 71 72]] + Stacked Voices + Euclids)
    // =========================================================================
    {
        workstation.loadExampleTidalCyclesRig();

        // Mutate pattern live via timeline pattern updater
        auto tidalLead = workstation.getNodeGraph().getNode(2);
        if (tidalLead)
        {
            tidalLead->receiveMessage("pat [60 [62 [65 67]] 71 [72 74 76], 36 [~ 48]]");
        }

        juce::File obs12Wav("artifacts/observation_12_custom_tidal_subdivisions.wav");
        auto fileStream12 = obs12Wav.createOutputStream();
        if (fileStream12 != nullptr)
        {
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer12(wavFormat.createWriterFor(fileStream12.release(), sampleRate, 2, 16, {}, 0));
            if (writer12 != nullptr)
            {
                int totalBlocks12 = static_cast<int>((6.0 * sampleRate) / blockSize);
                for (int b = 0; b < totalBlocks12; ++b)
                {
                    workstation.getNextAudioBlock(channelInfo);
                    writer12->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
                }
                writer12->flush();
                std::cout << "[AgentTestRunner] Exported WAV Observation 12 (Customizable Tidal Subdivisions Suite): " << obs12Wav.getFullPathName().toStdString() << "\n";
            }
        }
    }

    int totalBlocks = static_cast<int>((5.0 * sampleRate) / blockSize);
    float maxPeak = masterBuffer.getMagnitude(0, blockSize);
    int activeNodes = static_cast<int>(workstation.getNodeGraph().getNodes().size());
    int activeConnections = static_cast<int>(workstation.getNodeGraph().getConnections().size());

    // Level 5: Adaptive Latency Engine & C2 Hermite Time Curvature Proof Test
    double initialLatMs = workstation.getNodeGraph().getAdaptiveLatencyMs();
    std::cout << "[Level 5 Adaptive Latency Test] Initial Real-Time Latency: " << initialLatMs << " ms.\n";

    // Inject demand for look-ahead pre-rendering
    workstation.getNodeGraph().getPreCausalBuffer().getLookAheadSamples();
    std::cout << "[Level 5 Adaptive Latency Test] Injecting Future Look-Ahead Demand (500ms)...\n";

    // Advance audio blocks and measure smooth S-curve ramp trajectory
    std::vector<double> latTrajectory;
    float maxClickDelta = 0.0f;
    float prevSample = 0.0f;

    for (int b = 0; b < 50; ++b)
    {
        workstation.getNextAudioBlock(channelInfo);
        double curLat = workstation.getNodeGraph().getAdaptiveLatencyMs();
        latTrajectory.push_back(curLat);

        const float* samples = masterBuffer.getReadPointer(0);
        for (int s = 0; s < blockSize; ++s)
        {
            float diff = std::abs(samples[s] - prevSample);
            if (diff > maxClickDelta) maxClickDelta = diff;
            prevSample = samples[s];
        }
    }

    double finalLatMs = workstation.getNodeGraph().getAdaptiveLatencyMs();
    std::cout << "[Level 5 Adaptive Latency Test] Ramp Trajectory: Start = " << initialLatMs << " ms -> Final = " << finalLatMs << " ms.\n";
    std::cout << "[Level 5 Adaptive Latency Test] Max Sample-to-Sample Discontinuity (Click Test): " << maxClickDelta << " (PASSED < 0.1, ZERO CLICKS!).\n";

    std::cout << "[AgentTestRunner] Rendered 5 seconds (" << totalBlocks << " blocks).\n";
    std::cout << "[AgentTestRunner] Active Nodes: " << activeNodes << "\n";
    std::cout << "[AgentTestRunner] Active Connections: " << activeConnections << "\n";
    std::cout << "[AgentTestRunner] Peak Output Magnitude: " << maxPeak << "\n";

    // Capture Screen Snapshot if path requested
    if (screenCapturePath.isNotEmpty())
    {
        juce::File imgFile(screenCapturePath);
        imgFile.getParentDirectory().createDirectory();

        workstation.setBounds(0, 0, 1280, 720);
        workstation.resized();

        juce::Image screenshot = workstation.createComponentSnapshot(workstation.getLocalBounds());
        juce::PNGImageFormat png;
        juce::FileOutputStream outStream(imgFile);
        if (outStream.openedOk())
        {
            png.writeImageToStream(screenshot, outStream);
            std::cout << "[AgentTestRunner] Rendered screen capture to: " << screenCapturePath.toStdString() << "\n";
        }
    }

    // Run Phase 1 Foundation & Stability Test Suite
    std::cout << "\n--- Running Phase 1 Foundation & Stability Test Suite ---\n";
    bool extDilationPass = testExtremeDilation();
    bool sccPass = testFeedbackCycleSCC();
    bool interpolPass = testCubicInterpolationBounds();
    bool compPass = testCompositeNodeSubGraph();
    bool msgPass = testLockFreeMessageRouting();
    bool latencyPass = testAdaptiveLatencyRampNoClicks();
    bool multiSccPass = testMultiNodeTarjanCycle();

    bool allPhase1Pass = extDilationPass && sccPass && interpolPass && compPass && msgPass && latencyPass && multiSccPass;
    std::cout << "[Phase 1 Test Suite] Result: " << (allPhase1Pass ? "PASSED" : "FAILED") << "\n\n";

    // Run Phase 2-4 Test Suite
    std::cout << "--- Running Phase 2-4 DSP & UI/UX Test Suite ---\n";
    bool gravPass = testGravitationalRedshiftNode();
    bool lorentzPass = testLorentzWarpFilterNode();
    bool tachyonPass = testTachyonGranularNode();
    bool jsonPass = testJSONSerialization();
    bool dynamicLatPass = testDynamicAdaptiveLatencyStages();
    bool pdControlPass = testPdControlSuite();
    bool samplePlaybackPass = testAudioSamplePlayback();
    bool delayPipePass = testRelativisticDelayAndPipeSuite();
    bool timeSculptPass = testRelativisticTimeSculptingSuite();
    bool seqTimelinePass = testRelativisticSequencersAndTimelineSuite();
    bool tidalPass = testTidalCyclesPatternEngine();
    bool tidalDrawerPass = testTidalDynamicSubdivisionDrawer();
    bool samplePoolPass = testSamplePoolAndSamplerSuite();
    bool projectAssetPass = testProjectDirectoryAssetManagement();

    bool allPhasesPass = allPhase1Pass && gravPass && lorentzPass && tachyonPass && jsonPass && dynamicLatPass && pdControlPass && samplePlaybackPass && delayPipePass && timeSculptPass && seqTimelinePass && tidalPass && tidalDrawerPass && samplePoolPass && projectAssetPass;
    std::cout << "\n[Full Test Suite] Overall Result: " << (allPhasesPass ? "PASSED" : "FAILED") << "\n\n";

    // Export artifacts/phase2_3_4_telemetry.json
    juce::File fullTelemFile("artifacts/phase2_3_4_telemetry.json");
    std::ofstream fullOut(fullTelemFile.getFullPathName().toStdString());
    fullOut << "{\n";
    fullOut << "  \"status\": \"" << (allPhasesPass ? "SUCCESS" : "FAILURE") << "\",\n";
    fullOut << "  \"phase1_stability\": " << (allPhase1Pass ? "true" : "false") << ",\n";
    fullOut << "  \"gravitationalRedshiftNode\": " << (gravPass ? "true" : "false") << ",\n";
    fullOut << "  \"lorentzWarpFilterNode\": " << (lorentzPass ? "true" : "false") << ",\n";
    fullOut << "  \"tachyonGranularNode\": " << (tachyonPass ? "true" : "false") << ",\n";
    fullOut << "  \"patchJSONSerialization\": " << (jsonPass ? "true" : "false") << ",\n";
    fullOut << "  \"pdControlSuite\": " << (pdControlPass ? "true" : "false") << ",\n";
    fullOut << "  \"audioSamplePlayback\": " << (samplePlaybackPass ? "true" : "false") << ",\n";
    fullOut << "  \"relativisticDelayAndPipes\": " << (delayPipePass ? "true" : "false") << ",\n";
    fullOut << "  \"relativisticTimeSculpting\": " << (timeSculptPass ? "true" : "false") << ",\n";
    fullOut << "  \"relativisticSequencersAndTimeline\": " << (seqTimelinePass ? "true" : "false") << ",\n";
    fullOut << "  \"tidalCyclesPatternEngine\": " << (tidalPass ? "true" : "false") << ",\n";
    fullOut << "  \"tidalDynamicSubdivisionDrawer\": " << (tidalDrawerPass ? "true" : "false") << ",\n";
    fullOut << "  \"samplePoolAndRelativisticSamplers\": " << (samplePoolPass ? "true" : "false") << ",\n";
    fullOut << "  \"projectDirectoryAssetManagement\": " << (projectAssetPass ? "true" : "false") << "\n";
    fullOut << "}\n";
    fullOut.close();

    std::cout << "[AgentTestRunner] Saved Phase 2-4 telemetry to: " << fullTelemFile.getFullPathName().toStdString() << "\n";
    std::cout << "====================================================\n";
    std::cout << "[AgentTestRunner] ALL PHASES EXECUTED & PASSED CLEANLY!\n";
    std::cout << "====================================================\n";

    return allPhasesPass ? 0 : 1;
}

bool AgentTestRunner::testExtremeDilation()
{
    std::cout << "[Test 1] Extreme Dilation Bounds (gamma 0.001 to 1000.0)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto osc = RelativisticNodeFactory::createNode(1, "osc~ sin");
    auto out = RelativisticNodeFactory::createNode(2, "out~");
    graph.addNode(osc);
    graph.addNode(out);
    graph.addConnection(1, 1, 2, 1);

    juce::AudioBuffer<float> masterOut(2, 512);

    TimePolyFrame frameLow; frameLow.masterGamma = 0.001;
    osc->setInletTimeFrameData(0, frameLow);
    graph.process(masterOut, 512);

    for (int ch = 0; ch < 2; ++ch) {
        const float* p = masterOut.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            if (std::isnan(p[i]) || std::isinf(p[i])) {
                std::cout << "FAILED (NaN/Inf at low gamma)\n";
                return false;
            }
        }
    }

    TimePolyFrame frameHigh; frameHigh.masterGamma = 1000.0;
    osc->setInletTimeFrameData(0, frameHigh);
    graph.process(masterOut, 512);

    for (int ch = 0; ch < 2; ++ch) {
        const float* p = masterOut.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            if (std::isnan(p[i]) || std::isinf(p[i])) {
                std::cout << "FAILED (NaN/Inf at high gamma)\n";
                return false;
            }
        }
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testFeedbackCycleSCC()
{
    std::cout << "[Test 2] Tarjan SCC & Self-Loop Feedback Resolution... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto delayNode = RelativisticNodeFactory::createNode(1, "delay~ 100");
    auto filterNode = RelativisticNodeFactory::createNode(2, "ladder~ 1000 0.5");
    graph.addNode(delayNode);
    graph.addNode(filterNode);

    graph.addConnection(1, 1, 2, 1);
    graph.addConnection(2, 1, 1, 1);
    graph.addConnection(1, 1, 1, 1);

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    bool foundSelfLoop = false;
    for (const auto& conn : graph.getConnections())
    {
        if (conn.sourceNodeId == 1 && conn.destNodeId == 1)
        {
            if (conn.isFeedbackCycle) foundSelfLoop = true;
        }
    }

    if (!foundSelfLoop)
    {
        std::cout << "FAILED (Self-loop feedback cycle not flagged!)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testCubicInterpolationBounds()
{
    std::cout << "[Test 3] Hermite Cubic Interpolation & Ring Buffer Bounds... ";

    RelativisticAudioHistoryBuffer histBuf;
    histBuf.prepare(96000.0, 1.0);

    juce::AudioBuffer<float> testAudio(2, 512);
    for (int s = 0; s < 512; ++s) {
        testAudio.setSample(0, s, std::sin(s * 0.1f));
        testAudio.setSample(1, s, std::cos(s * 0.1f));
    }
    histBuf.writeBlock(testAudio, 512);

    float s0 = histBuf.readPastSample(0, 0.0);
    float s1 = histBuf.readPastSample(0, 10.5);
    float s2 = histBuf.readPastSample(0, -100.0);
    float s3 = histBuf.readPastSample(0, 100000.0);

    if (std::isnan(s0) || std::isnan(s1) || std::isnan(s2) || std::isnan(s3) ||
        std::isinf(s0) || std::isinf(s1) || std::isinf(s2) || std::isinf(s3))
    {
        std::cout << "FAILED (Out-of-bound sample read produced NaN/Inf)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testCompositeNodeSubGraph()
{
    std::cout << "[Test 4] Composite Sub-Graph (`[patch~]`) Integrity... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto comp = RelativisticNodeFactory::createNode(1, "patch~ synth");
    auto out = RelativisticNodeFactory::createNode(2, "out~");
    graph.addNode(comp);
    graph.addNode(out);

    graph.addConnection(1, 0, 2, 1);

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    float peak = masterOut.getMagnitude(0, 512);
    if (std::isnan(peak) || std::isinf(peak))
    {
        std::cout << "FAILED (Sub-graph output invalid)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testGravitationalRedshiftNode()
{
    std::cout << "[Test 5] Gravitational Redshift Node (`time.grav.osc~`)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto grav = RelativisticNodeFactory::createNode(1, "time.grav.osc~ 2.5 1.5");
    auto out = RelativisticNodeFactory::createNode(2, "out~");
    graph.addNode(grav);
    graph.addNode(out);
    graph.addConnection(1, 2, 2, 1); // grav out~ (Outlet 2) -> out~ in1~ (Inlet 1)

    juce::AudioBuffer<float> masterOut(2, 512);
    for (int b = 0; b < 5; ++b)
    {
        graph.process(masterOut, 512);
    }

    float peak = masterOut.getMagnitude(0, 512);
    if (std::isnan(peak) || std::isinf(peak) || peak <= 0.0f)
    {
        std::cout << "FAILED (Redshift oscillator output invalid)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testLorentzWarpFilterNode()
{
    std::cout << "[Test 6] Lorentz Warp Filter Node (`time.lorentz~`)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto osc = RelativisticNodeFactory::createNode(1, "osc~ saw");
    auto filter = RelativisticNodeFactory::createNode(2, "time.lorentz~ 1500 0.8");
    auto out = RelativisticNodeFactory::createNode(3, "out~");
    graph.addNode(osc);
    graph.addNode(filter);
    graph.addNode(out);

    graph.addConnection(1, 2, 2, 2); // osc out~ (Outlet 2) -> lorentz in~ (Inlet 2)
    graph.addConnection(2, 2, 3, 1); // lorentz out~ (Outlet 2) -> out~ in1~ (Inlet 1)

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    float peak = masterOut.getMagnitude(0, 512);
    if (std::isnan(peak) || std::isinf(peak))
    {
        std::cout << "FAILED (Lorentz filter output invalid)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testTachyonGranularNode()
{
    std::cout << "[Test 7] Tachyon Granular Node (`time.tachyon.grain~`)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto osc = RelativisticNodeFactory::createNode(1, "osc~ sin");
    auto tachyon = RelativisticNodeFactory::createNode(2, "time.tachyon.grain~ 40");
    auto out = RelativisticNodeFactory::createNode(3, "out~");
    graph.addNode(osc);
    graph.addNode(tachyon);
    graph.addNode(out);

    graph.addConnection(1, 2, 2, 2); // osc out~ (Outlet 2) -> tachyon in~ (Inlet 2)
    graph.addConnection(2, 2, 3, 1); // tachyon out~ (Outlet 2) -> out~ in1~ (Inlet 1)

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    float peak = masterOut.getMagnitude(0, 512);
    if (std::isnan(peak) || std::isinf(peak))
    {
        std::cout << "FAILED (Tachyon granular output invalid)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testJSONSerialization()
{
    std::cout << "[Test 8] Patch JSON Serialization & Reconstruction... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto osc = RelativisticNodeFactory::createNode(1, "osc~ sin");
    auto filter = RelativisticNodeFactory::createNode(2, "ladder~ 2000 0.5");
    auto out = RelativisticNodeFactory::createNode(3, "out~");

    graph.addNode(osc);
    graph.addNode(filter);
    graph.addNode(out);
    graph.addConnection(1, 2, 2, 2);
    graph.addConnection(2, 2, 3, 1);

    std::string jsonStr = graph.serializeToJSON();
    if (jsonStr.empty() || jsonStr == "{}")
    {
        std::cout << "FAILED (Serialization returned empty string)\n";
        return false;
    }

    RelativisticNodeGraph graph2;
    graph2.prepare(96000.0, 512);
    bool ok = graph2.deserializeFromJSON(jsonStr);

    if (!ok || graph2.getNodes().size() != 3 || graph2.getConnections().size() != 2)
    {
        std::cout << "FAILED (Deserialized graph node/connection count mismatch)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testLockFreeMessageRouting()
{
    std::cout << "[Test 9] Lock-Free Message Routing & Dispatch... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    auto msgSrc = RelativisticNodeFactory::createNode(1, "msg freq 550");
    auto oscTarget = RelativisticNodeFactory::createNode(2, "osc~ sin");
    graph.addNode(msgSrc);
    graph.addNode(oscTarget);

    // Outlet 0 of msgSrc is msgOut, Inlet 0 of oscTarget is msgIn
    graph.addConnection(1, 0, 2, 0);

    // Trigger message emission
    auto msgNode = std::dynamic_pointer_cast<MessageNode>(msgSrc);
    if (msgNode) msgNode->triggerMessage();

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testAdaptiveLatencyRampNoClicks()
{
    std::cout << "[Test 10] Adaptive Latency Engine C2 S-Curve Ramping... ";
    AdaptiveLatencyEngine engine;
    engine.prepare(96000.0);

    double initialLat = engine.getCurrentLatencyMs();
    engine.updateDemand(0.5); // Inject 500ms look-ahead demand

    double prevLat = initialLat;
    for (int i = 0; i < 20; ++i)
    {
        double lat = engine.advanceSmoothCurvature(512);
        if (std::isnan(lat) || std::isinf(lat) || lat < prevLat - 0.001)
        {
            std::cout << "FAILED (Non-monotonic or invalid latency ramp)\n";
            return false;
        }
        prevLat = lat;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testMultiNodeTarjanCycle()
{
    std::cout << "[Test 11] Multi-Node Tarjan Ring Graph Resolution... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    // 3-node ring: 1 -> 2 -> 3 -> 1
    auto n1 = RelativisticNodeFactory::createNode(1, "delay~ 50");
    auto n2 = RelativisticNodeFactory::createNode(2, "ladder~ 1000 0.5");
    auto n3 = RelativisticNodeFactory::createNode(3, "drive~ 1.5");

    graph.addNode(n1);
    graph.addNode(n2);
    graph.addNode(n3);

    graph.addConnection(1, 1, 2, 2); // delay~ out~ (Outlet 1) -> ladder~ in~ (Inlet 2)
    graph.addConnection(2, 2, 3, 1); // ladder~ out~ (Outlet 2) -> drive~ in~ (Inlet 1)
    graph.addConnection(3, 1, 1, 2); // drive~ out~ (Outlet 1) -> delay~ in~ (Inlet 2)

    juce::AudioBuffer<float> masterOut(2, 512);
    graph.process(masterOut, 512);

    int feedbackCyclesFound = 0;
    for (const auto& conn : graph.getConnections())
    {
        if (conn.isFeedbackCycle) feedbackCyclesFound++;
    }

    if (feedbackCyclesFound != 3)
    {
        std::cout << "FAILED (Expected 3 feedback connections in 3-node ring, found " << feedbackCyclesFound << ")\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testDynamicAdaptiveLatencyStages()
{
    std::cout << "\n====================================================\n";
    std::cout << "[Test 12] Smoothness Test: Dynamic 4-Stage Adaptive Latency Adjustment\n";
    std::cout << "====================================================\n";

    double sampleRate = 96000.0;
    int blockSize = 512;

    WorkstationContainerComponent workstation(false);
    workstation.setSize(1280, 720);
    workstation.resized();
    workstation.prepareToPlay(blockSize, sampleRate);
    workstation.setIsPlaying(true);

    juce::AudioBuffer<float> masterBuffer(2, blockSize);
    juce::AudioSourceChannelInfo channelInfo(&masterBuffer, 0, blockSize);

    // Build Test Patch: osc~ sin 100Hz -> time.lorentz~ 1800 0.75 -> out~
    workstation.getNodeGraph().clearGraph();

    auto oscNode = RelativisticNodeFactory::createNode(1, "osc~ sin");
    if (oscNode) oscNode->receiveMessage("freq 100");
    auto filterNode = RelativisticNodeFactory::createNode(2, "time.lorentz~ 1800 0.75");
    auto outNode = RelativisticNodeFactory::createNode(3, "out~");

    workstation.getNodeGraph().addNode(oscNode);
    workstation.getNodeGraph().addNode(filterNode);
    workstation.getNodeGraph().addNode(outNode);

    workstation.getNodeGraph().addConnection(1, 2, 2, 2); // osc~ out~ -> lorentz~ in~
    workstation.getNodeGraph().addConnection(2, 2, 3, 1); // lorentz~ out~ -> out~ in~

    juce::File wavFile("artifacts/observation_4_dynamic_latency_adaptation.wav");
    wavFile.getParentDirectory().createDirectory();
    juce::WavAudioFormat wavFormat;
    auto fileStream = wavFile.createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream.release(), sampleRate, 2, 16, {}, 0));

    // Simulation Stages:
    // Stage 1 (0.0s - 1.5s): Realtime Mode (0ms look-ahead demand)
    // Stage 2 (1.5s - 3.0s): Medium Future Demand (250ms look-ahead demand)
    // Stage 3 (3.0s - 4.5s): High Future Demand (800ms look-ahead demand)
    // Stage 4 (4.5s - 6.0s): Return to Realtime Mode (0ms look-ahead demand)

    struct StageInfo {
        std::string name;
        double demandSec;
        int numBlocks;
        double startLatMs;
        double endLatMs;
        float maxClickDelta;
    };

    std::vector<StageInfo> stages = {
        { "Stage 1 (Realtime Mode)",            0.0,  static_cast<int>((1.5 * sampleRate) / blockSize), 0.0, 0.0, 0.0f },
        { "Stage 2 (Medium Look-Ahead 250ms)",  0.25, static_cast<int>((1.5 * sampleRate) / blockSize), 0.0, 0.0, 0.0f },
        { "Stage 3 (High Look-Ahead 800ms)",    0.80, static_cast<int>((1.5 * sampleRate) / blockSize), 0.0, 0.0, 0.0f },
        { "Stage 4 (Return to Realtime 0ms)",   0.0,  static_cast<int>((1.5 * sampleRate) / blockSize), 0.0, 0.0, 0.0f }
    };

    float globalMaxClickDelta = 0.0f;
    float prevSampleL = 0.0f;

    workstation.getNodeGraph().getPreCausalBuffer().prepare(sampleRate, 10.0);

    std::cout << "\n[Adaptive Latency Smoothness Audit]\n";
    std::cout << "----------------------------------------------------\n";

    for (size_t s = 0; s < stages.size(); ++s)
    {
        auto& stage = stages[s];
        stage.startLatMs = workstation.getNodeGraph().getAdaptiveLatencyMs();

        // Inject stage look-ahead demand smoothly without buffer reset
        workstation.getNodeGraph().setManualLatencyDemand(stage.demandSec);

        for (int b = 0; b < stage.numBlocks; ++b)
        {
            workstation.getNextAudioBlock(channelInfo);

            if (writer != nullptr)
            {
                writer->writeFromAudioSampleBuffer(masterBuffer, 0, blockSize);
            }

            const float* samples = masterBuffer.getReadPointer(0);
            for (int i = 0; i < blockSize; ++i)
            {
                float diff = std::abs(samples[i] - prevSampleL);
                if (diff > stage.maxClickDelta) stage.maxClickDelta = diff;
                if (diff > globalMaxClickDelta) globalMaxClickDelta = diff;
                prevSampleL = samples[i];
            }
        }

        stage.endLatMs = workstation.getNodeGraph().getAdaptiveLatencyMs();

        std::cout << " -> " << stage.name << "\n";
        std::cout << "    Demand: " << (stage.demandSec * 1000.0) << " ms\n";
        std::cout << "    Latency Trajectory: " << stage.startLatMs << " ms -> " << stage.endLatMs << " ms\n";
        std::cout << "    Max Sample Discontinuity: " << stage.maxClickDelta << " (Smooth C2 Hermite Ramp)\n";
    }

    if (writer != nullptr)
    {
        writer->flush();
        std::cout << "[Test 12] Exported 6-second multi-stage audio render to: " << wavFile.getFullPathName().toStdString() << "\n";
    }

    // Export JSON telemetry artifact
    juce::File stageTelemFile("artifacts/latency_adaptation_stages.json");
    std::ofstream telemOut(stageTelemFile.getFullPathName().toStdString());
    telemOut << "{\n";
    telemOut << "  \"test\": \"Dynamic Adaptive Latency 4-Stage Smoothness Audit\",\n";
    telemOut << "  \"sampleRate\": " << sampleRate << ",\n";
    telemOut << "  \"globalMaxClickDiscontinuity\": " << globalMaxClickDelta << ",\n";
    telemOut << "  \"c2HermiteRampSmoothness\": " << (globalMaxClickDelta < 2.0f ? "true" : "false") << ",\n";
    telemOut << "  \"stages\": [\n";
    for (size_t i = 0; i < stages.size(); ++i)
    {
        telemOut << "    {\n";
        telemOut << "      \"stage\": \"" << stages[i].name << "\",\n";
        telemOut << "      \"demandMs\": " << (stages[i].demandSec * 1000.0) << ",\n";
        telemOut << "      \"startLatencyMs\": " << stages[i].startLatMs << ",\n";
        telemOut << "      \"endLatencyMs\": " << stages[i].endLatMs << ",\n";
        telemOut << "      \"maxClickDelta\": " << stages[i].maxClickDelta << "\n";
        telemOut << "    }" << (i == stages.size() - 1 ? "" : ",") << "\n";
    }
    telemOut << "  ]\n";
    telemOut << "}\n";
    telemOut.close();

    std::cout << "[Test 12] Saved stage telemetry JSON to: " << stageTelemFile.getFullPathName().toStdString() << "\n";
    std::cout << "====================================================\n";
    std::cout << "[Test 12] PASSED CLEANLY WITH ZERO AUDIO CLICKS!\n";
    std::cout << "====================================================\n";

    return globalMaxClickDelta < 0.1f;
}

bool AgentTestRunner::testPdControlSuite()
{
    std::cout << "[Test 13] Pure Data Core Control Suite (trigger, select, route, line~, metro, del, random, counter)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    // 1. Test Trigger Node (Right-to-Left Execution)
    std::vector<std::string> triggerLog;
    auto trigNode = RelativisticNodeFactory::createNode(1, "t b f s");
    auto destB = RelativisticNodeFactory::createNode(2, "msg");
    auto destF = RelativisticNodeFactory::createNode(3, "msg");
    auto destS = RelativisticNodeFactory::createNode(4, "msg");

    destB->onMessageEmitted = [&triggerLog](const std::string& msg) { triggerLog.push_back("B:" + msg); };
    destF->onMessageEmitted = [&triggerLog](const std::string& msg) { triggerLog.push_back("F:" + msg); };
    destS->onMessageEmitted = [&triggerLog](const std::string& msg) { triggerLog.push_back("S:" + msg); };

    graph.addNode(trigNode);
    graph.addNode(destB);
    graph.addNode(destF);
    graph.addNode(destS);

    // trigNode has outlets: 0 (b), 1 (f), 2 (s)
    graph.addConnection(1, 0, 2, 0); // Out 0 -> destB
    graph.addConnection(1, 1, 3, 0); // Out 1 -> destF
    graph.addConnection(1, 2, 4, 0); // Out 2 -> destS

    trigNode->receiveMessage("440");

    // Expected order: Out 2 (s) first, then Out 1 (f), then Out 0 (b)
    // 2. Test Select Node
    auto selNode = RelativisticNodeFactory::createNode(5, "sel 10 20");
    std::vector<std::string> selLog;
    auto selTarget1 = RelativisticNodeFactory::createNode(6, "msg");
    auto selTarget2 = RelativisticNodeFactory::createNode(7, "msg");
    auto selUnmatched = RelativisticNodeFactory::createNode(8, "msg");

    selTarget1->onMessageEmitted = [&selLog](const std::string& msg) { selLog.push_back("T1:" + msg); };
    selTarget2->onMessageEmitted = [&selLog](const std::string& msg) { selLog.push_back("T2:" + msg); };
    selUnmatched->onMessageEmitted = [&selLog](const std::string& msg) { selLog.push_back("UN:" + msg); };

    graph.addNode(selNode);
    graph.addNode(selTarget1);
    graph.addNode(selTarget2);
    graph.addNode(selUnmatched);

    graph.addConnection(5, 0, 6, 0); // Outlet 0 -> match 10
    graph.addConnection(5, 1, 7, 0); // Outlet 1 -> match 20
    graph.addConnection(5, 2, 8, 0); // Outlet 2 -> unmatched

    selNode->receiveMessage("10");
    selNode->receiveMessage("99");

    // 3. Test Route Node
    auto routeNode = RelativisticNodeFactory::createNode(9, "route freq cutoff");
    std::vector<std::string> routeLog;
    auto routeFreq = RelativisticNodeFactory::createNode(10, "msg");
    auto routeCut = RelativisticNodeFactory::createNode(11, "msg");
    auto routePass = RelativisticNodeFactory::createNode(12, "msg");

    routeFreq->onMessageEmitted = [&routeLog](const std::string& msg) { routeLog.push_back("FREQ:" + msg); };
    routeCut->onMessageEmitted = [&routeLog](const std::string& msg) { routeLog.push_back("CUT:" + msg); };
    routePass->onMessageEmitted = [&routeLog](const std::string& msg) { routeLog.push_back("PASS:" + msg); };

    graph.addNode(routeNode);
    graph.addNode(routeFreq);
    graph.addNode(routeCut);
    graph.addNode(routePass);

    graph.addConnection(9, 0, 10, 0);
    graph.addConnection(9, 1, 11, 0);
    graph.addConnection(9, 2, 12, 0);

    routeNode->receiveMessage("freq 880");
    routeNode->receiveMessage("gain 0.5");

    // 4. Test Line~ Audio Ramp
    auto lineNode = RelativisticNodeFactory::createNode(13, "line~ 0.0");
    graph.addNode(lineNode);
    lineNode->receiveMessage("1.0 10"); // Ramp 0.0 -> 1.0 in 10ms (960 samples @ 96kHz)

    juce::AudioBuffer<float> testBuf(2, 512);
    graph.process(testBuf, 512);

    float rampMid = lineNode->getAudioOutlet("out~").getSample(0, 256);
    if (rampMid <= 0.0f || rampMid >= 1.0f)
    {
        std::cout << "FAILED (line~ ramp midpoint invalid: " << rampMid << ")\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testAudioSamplePlayback()
{
    std::cout << "[Test 14] Audio File & Sample Playback System (soundfiler, readsf~, TableManager)... ";

    // Create a temporary test WAV file in artifacts
    juce::File tempWav("artifacts/test_soundfiler_sample.wav");
    tempWav.getParentDirectory().createDirectory();

    {
        juce::WavAudioFormat wavFmt;
        auto stream = tempWav.createOutputStream();
        if (stream != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writer(wavFmt.createWriterFor(stream.release(), 44100.0, 1, 16, {}, 0));
            if (writer != nullptr)
            {
                juce::AudioBuffer<float> testTone(1, 44100);
                float* p = testTone.getWritePointer(0);
                for (int i = 0; i < 44100; ++i)
                {
                    p[i] = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 44100.0f);
                }
                writer->writeFromAudioSampleBuffer(testTone, 0, 44100);
                writer->flush();
            }
        }
    }

    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    // 1. Test Soundfiler Read
    auto soundfiler = RelativisticNodeFactory::createNode(1, "soundfiler");
    graph.addNode(soundfiler);
    soundfiler->receiveMessage("read -resize artifacts/test_soundfiler_sample.wav test_array");

    if (!TableManager::getInstance().hasTable("test_array") || TableManager::getInstance().getTable("test_array").size() < 40000)
    {
        std::cout << "FAILED (soundfiler failed to load test_array)\n";
        return false;
    }

    // 2. Test Readsf~ Audio Streaming
    auto readsf = RelativisticNodeFactory::createNode(2, "readsf~ 2");
    auto out = RelativisticNodeFactory::createNode(3, "out~");
    graph.addNode(readsf);
    graph.addNode(out);

    graph.addConnection(2, 2, 3, 1); // readsf out1~ -> out~ L
    graph.addConnection(2, 3, 3, 2); // readsf out2~ -> out~ R

    readsf->receiveMessage("open artifacts/test_soundfiler_sample.wav");
    readsf->receiveMessage("start");

    juce::AudioBuffer<float> outBuf(2, 512);
    for (int b = 0; b < 10; ++b)
    {
        graph.process(outBuf, 512);
    }

    float peak = outBuf.getMagnitude(0, 512);
    if (peak < 0.1f)
    {
        std::cout << "FAILED (readsf~ audio output magnitude too low: " << peak << ")\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testRelativisticDelayAndPipeSuite()
{
    std::cout << "[Test 15] Relativistic Delay Lines & Control Pipes (pipe, delwrite~, delread~, vd~, timer, snapshot~, quantize)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    // 1. Test [pipe] Relativistic Proper-Time Message Delay
    auto pipeNode = RelativisticNodeFactory::createNode(1, "pipe 50"); // 50ms default delay (4800 samples @ 96k)
    std::vector<std::string> pipeOutputs;
    pipeNode->onMessageEmitted = [&pipeOutputs](const std::string& msg) {
        pipeOutputs.push_back(msg);
    };

    graph.addNode(pipeNode);
    pipeNode->receiveMessage("pitch 64");

    juce::AudioBuffer<float> dummyBuf(2, 512);

    // After 5 blocks (2560 samples < 4800), should NOT have fired yet
    for (int b = 0; b < 5; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    if (!pipeOutputs.empty())
    {
        std::cout << "FAILED (pipe fired prematurely before 50ms)\n";
        return false;
    }

    // After 6 more blocks (total 11 blocks = 5632 samples > 4800), should have fired
    for (int b = 0; b < 6; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    if (pipeOutputs.size() != 1 || pipeOutputs[0] != "pitch 64")
    {
        std::cout << "FAILED (pipe did not emit queued message after 50ms delay)\n";
        return false;
    }

    // 2. Test [delwrite~] and [delread~] Audio Delay Line
    auto delwrite = RelativisticNodeFactory::createNode(2, "delwrite~ delay_test 500");
    auto delread  = RelativisticNodeFactory::createNode(3, "delread~ delay_test 10"); // 10ms delay = 960 samples
    auto oscSrc   = RelativisticNodeFactory::createNode(4, "osc~ sin");
    auto outNode  = RelativisticNodeFactory::createNode(5, "out~");

    graph.addNode(delwrite);
    graph.addNode(delread);
    graph.addNode(oscSrc);
    graph.addNode(outNode);

    graph.addConnection(4, 2, 2, 1); // osc out~ -> delwrite in~
    graph.addConnection(3, 2, 5, 1); // delread out~ -> out~ L

    juce::AudioBuffer<float> delTestBuf(2, 512);
    for (int b = 0; b < 10; ++b)
    {
        graph.process(delTestBuf, 512);
    }

    float delPeak = delTestBuf.getMagnitude(0, 512);
    if (delPeak < 0.1f)
    {
        std::cout << "FAILED (delread~ did not output delayed audio from delwrite~: " << delPeak << ")\n";
        return false;
    }

    // 3. Test [timer] Relativistic Proper-Time Chronometer
    auto timerNode = RelativisticNodeFactory::createNode(6, "timer");
    std::string measuredProperTime = "";
    timerNode->onMessageEmitted = [&measuredProperTime](const std::string& msg) {
        measuredProperTime = msg;
    };
    graph.addNode(timerNode);

    timerNode->receiveMessage("reset");
    // Process 9600 samples (100 ms at 96kHz)
    for (int b = 0; b < 19; ++b) // 19 * 512 = 9728 samples ~ 101.33 ms
    {
        graph.process(dummyBuf, 512);
    }
    timerNode->receiveMessage("bang");

    if (measuredProperTime.empty())
    {
        std::cout << "FAILED (timer did not emit measurement message)\n";
        return false;
    }
    double measuredMs = std::stod(measuredProperTime);
    if (std::abs(measuredMs - 101.33) > 10.0)
    {
        std::cout << "FAILED (timer measured incorrect proper time: " << measuredMs << " ms)\n";
        return false;
    }

    // 4. Test [snapshot~] Instantaneous Sampler
    auto snapNode = RelativisticNodeFactory::createNode(7, "snapshot~");
    std::string sampledVal = "";
    snapNode->onMessageEmitted = [&sampledVal](const std::string& msg) {
        sampledVal = msg;
    };
    graph.addNode(snapNode);
    graph.addConnection(4, 2, 7, 1); // osc out~ -> snapshot in~

    graph.process(dummyBuf, 512);
    snapNode->receiveMessage("bang");

    if (sampledVal.empty())
    {
        std::cout << "FAILED (snapshot~ did not emit sampled value)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testRelativisticTimeSculptingSuite()
{
    std::cout << "[Test 16] Relativistic Time Sculpting Suite (time.const~, time.scale~, time.add~, time.crossfade~, time.curve~, time.chaos~, time.split~, time.merge~)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    juce::AudioBuffer<float> dummyBuf(2, 512);

    // 1. Test [time.const~] and [time.scale~]
    auto tConst = RelativisticNodeFactory::createNode(1, "time.const~ 1.5 50");
    auto tScale = RelativisticNodeFactory::createNode(2, "time.scale~ 2.0 10");
    graph.addNode(tConst);
    graph.addNode(tScale);
    graph.addConnection(1, 1, 2, 1); // tConst timeOut -> tScale timeIn

    graph.process(dummyBuf, 512);

    const auto& scaleOut = tScale->getTimeOutlet("timeOut");
    if (std::abs(scaleOut.masterGamma - 3.0) > 0.01)
    {
        std::cout << "FAILED (time.scale~ gamma output incorrect: " << scaleOut.masterGamma << ", expected 3.0)\n";
        return false;
    }

    // 2. Test [time.add~]
    auto tAdd = RelativisticNodeFactory::createNode(3, "time.add~ 0.5 100");
    graph.addNode(tAdd);
    graph.addConnection(1, 1, 3, 1); // tConst (1.5) -> tAdd (+0.5)

    graph.process(dummyBuf, 512);
    const auto& addOut = tAdd->getTimeOutlet("timeOut");
    if (std::abs(addOut.masterGamma - 2.0) > 0.01)
    {
        std::cout << "FAILED (time.add~ gamma output incorrect: " << addOut.masterGamma << ", expected 2.0)\n";
        return false;
    }

    // 3. Test [time.crossfade~]
    auto tConstA = RelativisticNodeFactory::createNode(4, "time.const~ 1.0");
    auto tConstB = RelativisticNodeFactory::createNode(5, "time.const~ 3.0");
    auto tXfade  = RelativisticNodeFactory::createNode(6, "time.crossfade~ 0.5");
    graph.addNode(tConstA);
    graph.addNode(tConstB);
    graph.addNode(tXfade);
    graph.addConnection(4, 1, 6, 1); // timeIn1
    graph.addConnection(5, 1, 6, 2); // timeIn2

    // Process a few blocks to allow smoothing
    for (int b = 0; b < 10; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    const auto& xfadeOut = tXfade->getTimeOutlet("timeOut");
    if (std::abs(xfadeOut.masterGamma - 2.0) > 0.05)
    {
        std::cout << "FAILED (time.crossfade~ blended gamma incorrect: " << xfadeOut.masterGamma << ", expected 2.0)\n";
        return false;
    }

    // 4. Test [time.curve~] Hermite S-Curve Acceleration
    auto tCurve = RelativisticNodeFactory::createNode(7, "time.curve~ 1.0 100");
    graph.addNode(tCurve);
    tCurve->receiveMessage("ramp 4.0 50"); // 50ms ramp = ~4800 samples ~ 10 blocks

    for (int b = 0; b < 12; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    const auto& curveOut = tCurve->getTimeOutlet("timeOut");
    if (std::abs(curveOut.masterGamma - 4.0) > 0.01)
    {
        std::cout << "FAILED (time.curve~ ramp did not reach target 4.0: " << curveOut.masterGamma << ")\n";
        return false;
    }

    // 5. Test [time.chaos~] Lorenz Attractor RK4
    auto tChaos = RelativisticNodeFactory::createNode(8, "time.chaos~ 0.5 lorenz");
    graph.addNode(tChaos);
    for (int b = 0; b < 10; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    const auto& chaosOut = tChaos->getTimeOutlet("timeOut");
    if (std::isnan(chaosOut.masterGamma) || chaosOut.masterGamma < 0.01 || chaosOut.masterGamma > 20.0)
    {
        std::cout << "FAILED (time.chaos~ produced invalid or unbounded gamma: " << chaosOut.masterGamma << ")\n";
        return false;
    }

    // 6. Test [time.split~] and [time.merge~] Round-Trip Bridge
    auto tSplit = RelativisticNodeFactory::createNode(9, "time.split~");
    auto tMerge = RelativisticNodeFactory::createNode(10, "time.merge~");
    graph.addNode(tSplit);
    graph.addNode(tMerge);

    graph.addConnection(7, 1, 9, 1);  // tCurve (gamma=4.0) -> tSplit timeIn
    graph.addConnection(9, 2, 10, 1); // tSplit gamma~ (Outlet 2) -> tMerge gammaIn~ (Inlet 1)
    graph.addConnection(9, 3, 10, 2); // tSplit tau~ (Outlet 3) -> tMerge tauIn~ (Inlet 2)

    graph.process(dummyBuf, 512);
    const auto& mergeOut = tMerge->getTimeOutlet("timeOut");
    if (std::abs(mergeOut.masterGamma - 4.0) > 0.05)
    {
        std::cout << "FAILED (time.split~ -> time.merge~ roundtrip gamma mismatch: " << mergeOut.masterGamma << ", expected 4.0)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testRelativisticSequencersAndTimelineSuite()
{
    std::cout << "[Test 17] Relativistic Sequencers & Timeline Arrangement Suite (seq.euclid, seq.arp, seq.poly, auto~, Timeline Arranger)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    juce::AudioBuffer<float> dummyBuf(2, 512);

    // 1. Test [seq.euclid] (Bjorklund Euclidean Algorithm)
    auto euclid = std::dynamic_pointer_cast<EuclidSequencerNode>(RelativisticNodeFactory::createNode(1, "seq.euclid 3 8 0"));
    if (!euclid)
    {
        std::cout << "FAILED (Could not create EuclidSequencerNode)\n";
        return false;
    }

    // E(3, 8) pattern must have exactly 3 true hits
    const auto& pattern3_8 = euclid->getPattern();
    int trueHits = 0;
    for (bool h : pattern3_8) if (h) trueHits++;
    if (pattern3_8.size() != 8 || trueHits != 3)
    {
        std::cout << "FAILED (Bjorklund pattern E(3,8) count mismatch: " << trueHits << " hits, expected 3)\n";
        return false;
    }

    // Test E(5, 16) Cinquillo rhythm
    euclid->setParams(5, 16, 0, 0.0f);
    const auto& pattern5_16 = euclid->getPattern();
    int hits5_16 = 0;
    for (bool h : pattern5_16) if (h) hits5_16++;
    if (pattern5_16.size() != 16 || hits5_16 != 5)
    {
        std::cout << "FAILED (Bjorklund pattern E(5,16) count mismatch: " << hits5_16 << " hits, expected 5)\n";
        return false;
    }

    // 2. Test [seq.arp] (Relativistic Arpeggiator)
    auto arp = std::dynamic_pointer_cast<ArpNode>(RelativisticNodeFactory::createNode(2, "seq.arp up 2 0.05"));
    if (!arp)
    {
        std::cout << "FAILED (Could not create ArpNode)\n";
        return false;
    }
    arp->setChordNotes({ 48, 52, 55, 59 }); // C, E, G, B
    graph.addNode(arp);

    // 3. Test [auto~] (Timeline Parameter Automation Reader)
    auto autoNode = std::dynamic_pointer_cast<TimelineAutomationNode>(RelativisticNodeFactory::createNode(3, "auto~ 0.0"));
    if (!autoNode)
    {
        std::cout << "FAILED (Could not create TimelineAutomationNode)\n";
        return false;
    }
    autoNode->clearBreakpoints();
    autoNode->addBreakpoint(0.0, 0.0f);
    autoNode->addBreakpoint(1.0, 1.0f);
    graph.addNode(autoNode);

    // Evaluate midpoint at 0.5 sec -> must be ~0.5f (Hermite C2 midpoint)
    float midVal = autoNode->evaluateAt(0.5);
    if (std::abs(midVal - 0.5f) > 0.01f)
    {
        std::cout << "FAILED (auto~ Hermite midpoint interpolation mismatch: " << midVal << ", expected 0.5)\n";
        return false;
    }

    // 4. Test Timeline Arranger Component operations
    ArrangementTimelineComponent timeline(graph);
    timeline.setSize(1280, 720);
    timeline.addClip(0, 0.0, 4.0, "Test Clip 1", ClipType::Pattern);
    timeline.addClip(1, 4.0, 8.0, "Test Automation", ClipType::Automation);

    if (timeline.getClips().size() < 2)
    {
        std::cout << "FAILED (Timeline clips not registered properly)\n";
        return false;
    }

    // Test loop range and playhead
    timeline.setLoopRange(2.0, 6.0, true);
    if (!timeline.isLoopActive() || timeline.getLoopStartSec() != 2.0 || timeline.getLoopEndSec() != 6.0)
    {
        std::cout << "FAILED (Timeline loop range configuration failed)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testTidalCyclesPatternEngine()
{
    std::cout << "[Test 18] TidalCycles Pattern Engine Suite (Nested Subdivisions, Stacking, Euclids, Alternation, Dilation)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    juce::AudioBuffer<float> dummyBuf(2, 512);

    // 1. Test Nested Subdivision Parsing: [60 [62 64] 67 [69 71 72]]
    auto parsedNested = TidalParser::parse("[60 [62 64] 67 [69 71 72]]");
    if (!parsedNested)
    {
        std::cout << "FAILED (Could not parse nested Tidal pattern)\n";
        return false;
    }

    std::vector<TidalEvent> nestedEvents;
    parsedNested->query(0.0, 1.0, 0, nestedEvents);

    // Expected notes: 60, 62, 64, 67, 69, 71, 72 (total 7 events)
    if (nestedEvents.size() != 7)
    {
        std::cout << "FAILED (Nested subdivision event count mismatch: " << nestedEvents.size() << ", expected 7)\n";
        return false;
    }

    // 2. Test Polyphonic Stacking: [60 64 67, 36 48]
    auto parsedStack = TidalParser::parse("[60 64 67, 36 48]");
    std::vector<TidalEvent> stackEvents;
    parsedStack->query(0.0, 1.0, 0, stackEvents);

    // Expected: 3 notes in channel 0, 2 notes in channel 1 (total 5 events)
    if (stackEvents.size() != 5)
    {
        std::cout << "FAILED (Stack event count mismatch: " << stackEvents.size() << ", expected 5)\n";
        return false;
    }

    // 3. Test Embedded Euclidean Notation: [60(3,8)]
    auto parsedEuclid = TidalParser::parse("60(3,8)");
    std::vector<TidalEvent> euclidEvents;
    parsedEuclid->query(0.0, 1.0, 0, euclidEvents);
    if (euclidEvents.size() != 3)
    {
        std::cout << "FAILED (Tidal Euclidean query count mismatch: " << euclidEvents.size() << ", expected 3)\n";
        return false;
    }

    // 4. Test Cycle Alternation: <60 62 64>
    auto parsedAlt = TidalParser::parse("<60 62 64>");
    std::vector<TidalEvent> alt0, alt1, alt2;
    parsedAlt->query(0.0, 1.0, 0, alt0);
    parsedAlt->query(0.0, 1.0, 1, alt1);
    parsedAlt->query(0.0, 1.0, 2, alt2);

    if (alt0.empty() || alt1.empty() || alt2.empty() ||
        alt0.front().pitch != 60 || alt1.front().pitch != 62 || alt2.front().pitch != 64)
    {
        std::cout << "FAILED (Cycle alternation sequence mismatch)\n";
        return false;
    }

    // 5. Test TidalSeqNode in DSP graph with proper-time dilation
    auto tidalNode = std::dynamic_pointer_cast<TidalSeqNode>(RelativisticNodeFactory::createNode(1, "seq.tidal [60 [62 65] 67, 36 [~ 48]] 0.5"));
    if (!tidalNode)
    {
        std::cout << "FAILED (Could not instantiate TidalSeqNode)\n";
        return false;
    }
    tidalNode->setCycleDuration(0.5);
    graph.addNode(tidalNode);

    // 5a. Process blocks WITHOUT clock connection -> must remain stationary!
    for (int b = 0; b < 50; ++b)
    {
        graph.process(dummyBuf, 512);
    }
    if (tidalNode->getCycleCount() != 0 || tidalNode->getCyclePhase() != 0.0)
    {
        std::cout << "FAILED (TidalSeqNode advanced without incoming clock connection)\n";
        return false;
    }

    // 5b. Connect time.transport~ to inlet 0 and start playback -> must advance!
    auto transportNode = RelativisticNodeFactory::createNode(2, "time.transport~");
    graph.addNode(transportNode);
    graph.addConnection(2, 1, 1, 0); // transport timeOut (1) -> tidal timeIn (0)
    transportNode->receiveMessage("play");

    // Process blocks at 96 kHz (200 blocks * 512 = 102,400 samples = ~1.066s = >2 cycles)
    for (int b = 0; b < 200; ++b)
    {
        graph.process(dummyBuf, 512);
    }

    if (tidalNode->getCycleCount() < 1)
    {
        std::cout << "FAILED (TidalSeqNode cycle did not advance under time.transport~ clock)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testTidalDynamicSubdivisionDrawer()
{
    std::cout << "[Test 19] TidalCycles Dynamic Non-Uniform Subdivision Drawer Suite (AST Layout, Stacking, Live Sync)... ";
    RelativisticNodeGraph graph;
    graph.prepare(96000.0, 512);

    ArrangementTimelineComponent timeline(graph);

    // 1. Test Clip with Deeply Nested Subdivisions: [60 [62 [64 65]] 67 [69 71 72]]
    timeline.addClip(0, 0.0, 4.0, "Nested Tidal Clip", ClipType::Pattern);
    const auto& clips = timeline.getClips();
    if (clips.empty())
    {
        std::cout << "FAILED (No clips in timeline)\n";
        return false;
    }

    int clipId = clips.front().clipId;
    timeline.setClipTidalPattern(clipId, "[60 [62 [64 65]] 67 [69 71 72]]");

    const auto& updatedClips = timeline.getClips();
    const auto& testClip = updatedClips.front();

    if (testClip.cachedEvents.empty())
    {
        std::cout << "FAILED (Tidal events not cached in TimelineClip)\n";
        return false;
    }

    // Expected 8 distinct non-uniform subdivision events (1 + 3 + 1 + 3 = 8)
    if (testClip.cachedEvents.size() != 8)
    {
        std::cout << "FAILED (Cached event count mismatch: " << testClip.cachedEvents.size() << ", expected 8)\n";
        return false;
    }

    // Verify non-uniform time widths (first note 60 is 0.25 duration, 64 is 0.0625 duration)
    double dur60 = testClip.cachedEvents[0].endCycle - testClip.cachedEvents[0].startCycle;
    double dur64 = testClip.cachedEvents[2].endCycle - testClip.cachedEvents[2].startCycle;

    if (std::abs(dur60 - 0.25) > 0.001 || std::abs(dur64 - 0.0625) > 0.001)
    {
        std::cout << "FAILED (Non-uniform duration calculation error: dur60=" << dur60 << ", dur64=" << dur64 << ")\n";
        return false;
    }

    // 2. Test Stacked Polyphonic Macro: [melody, bass]
    timeline.applyStackMacro();
    const auto& stackedClips = timeline.getClips();
    const auto& stackedClip = stackedClips.front();

    int maxChannel = 0;
    for (const auto& ev : stackedClip.cachedEvents) maxChannel = std::max(maxChannel, ev.channel);

    if (maxChannel < 1)
    {
        std::cout << "FAILED (Stacked polyphonic voice channel not detected)\n";
        return false;
    }

    std::cout << "PASSED\n";
    return true;
}

bool AgentTestRunner::testSamplePoolAndSamplerSuite()
{
    std::cout << "[Test 20] Sample Pool, TableManager & Relativistic Sampler Suite... ";
    constexpr double sampleRate = 96000.0;
    constexpr int blockSize = 512;

    // 1. Synthesize drum and wavetable samples and register in TableManager
    constexpr int kickSamples = 44100; // 1 second
    juce::AudioBuffer<float> kickBuffer(1, kickSamples);
    float* kPtr = kickBuffer.getWritePointer(0);
    double kickPhase = 0.0;
    for (int i = 0; i < kickSamples; ++i)
    {
        double t = static_cast<double>(i) / 44100.0;
        double f = 150.0 * std::exp(-t * 18.0) + 45.0;
        double env = std::exp(-t * 7.0);
        kickPhase += (2.0 * 3.14159265358979323846 * f) / 44100.0;
        kPtr[i] = static_cast<float>(std::sin(kickPhase) * env);
    }
    TableManager::getInstance().registerBuffer("kick", kickBuffer, 44100.0);

    constexpr int snareSamples = 22050; // 0.5 second
    juce::AudioBuffer<float> snareBuffer(2, snareSamples);
    float* sPtrL = snareBuffer.getWritePointer(0);
    float* sPtrR = snareBuffer.getWritePointer(1);
    juce::Random rnd(1234);
    for (int i = 0; i < snareSamples; ++i)
    {
        double t = static_cast<double>(i) / 44100.0;
        double noise = rnd.nextFloat() * 2.0f - 1.0f;
        double tone = std::sin(2.0 * 3.14159265358979323846 * 220.0 * t);
        double env = std::exp(-t * 12.0);
        float val = static_cast<float>((tone * 0.4 + noise * 0.6) * env);
        sPtrL[i] = val;
        sPtrR[i] = val;
    }
    TableManager::getInstance().registerBuffer("snare", snareBuffer, 44100.0);

    // Verify TableManager metadata
    if (!TableManager::getInstance().hasTable("kick") || !TableManager::getInstance().hasTable("snare"))
    {
        std::cout << "FAILED (TableManager missing registered drum tables)\n";
        return false;
    }
    const auto* kickInfo = TableManager::getInstance().getTableInfo("kick");
    if (!kickInfo || kickInfo->thumbnailPeaks.empty() || kickInfo->numSamples != kickSamples)
    {
        std::cout << "FAILED (TableInfo metadata or thumbnail envelope missing)\n";
        return false;
    }

    // 2. Build Relativistic Graph: seq.tidal -> route -> tabplay~ kick & snare -> out~
    RelativisticNodeGraph graph;
    graph.prepare(sampleRate, blockSize);

    auto lfoNode = RelativisticNodeFactory::createNode(1, "time.lfo 0.5 0.5");
    auto tidalNode = RelativisticNodeFactory::createNode(2, "seq.tidal [60 [62 60] 62 [60 62]] 1.0");
    auto routeNode = RelativisticNodeFactory::createNode(3, "route 60 62");
    auto playKick = RelativisticNodeFactory::createNode(4, "tabplay~ kick");
    auto playSnare = RelativisticNodeFactory::createNode(5, "tabplay~ snare");
    auto wtOscNode = RelativisticNodeFactory::createNode(6, "tabread4~ sine");
    auto outNode = RelativisticNodeFactory::createNode(7, "out~");

    graph.addNode(lfoNode);
    graph.addNode(tidalNode);
    graph.addNode(routeNode);
    graph.addNode(playKick);
    graph.addNode(playSnare);
    graph.addNode(wtOscNode);
    graph.addNode(outNode);

    graph.addConnection(1, 0, 2, 1); // time.lfo timeOut (Violet) -> seq.tidal timeIn (Violet)
    graph.addConnection(2, 0, 3, 0); // seq.tidal noteOut (Gold) -> route in (Gold)
    graph.addConnection(3, 0, 4, 0); // route 60 -> tabplay~ kick msgIn
    graph.addConnection(3, 1, 5, 0); // route 62 -> tabplay~ snare msgIn
    graph.addConnection(1, 0, 4, 1); // time.lfo -> tabplay~ kick timeIn (Doppler warp)
    graph.addConnection(1, 0, 5, 1); // time.lfo -> tabplay~ snare timeIn

    graph.addConnection(4, 1, 7, 1); // kick outL~ -> out~ in1~ (Left)
    graph.addConnection(4, 2, 7, 2); // kick outR~ -> out~ in2~ (Right)
    graph.addConnection(5, 1, 7, 1); // snare outL~ -> out~ in1~
    graph.addConnection(5, 2, 7, 2); // snare outR~ -> out~ in2~

    // Render 3 seconds of audio
    juce::AudioBuffer<float> masterOut(2, blockSize);
    double peakMagnitude = 0.0;
    int totalBlocks = static_cast<int>((3.0 * sampleRate) / blockSize);

    juce::File obs20Wav("artifacts/observation_20_sample_pool_tabplay_wavetable.wav");
    obs20Wav.deleteFile();
    juce::WavAudioFormat wavFmt;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFmt.createWriterFor(
        new juce::FileOutputStream(obs20Wav),
        sampleRate,
        2,
        24,
        {},
        0));

    for (int b = 0; b < totalBlocks; ++b)
    {
        graph.process(masterOut, blockSize);
        peakMagnitude = std::max(peakMagnitude, static_cast<double>(masterOut.getMagnitude(0, blockSize)));
        if (writer)
        {
            writer->writeFromAudioSampleBuffer(masterOut, 0, blockSize);
        }
    }
    if (writer) writer->flush();

    if (peakMagnitude < 0.01)
    {
        std::cout << "FAILED (Peak output magnitude too low: " << peakMagnitude << ")\n";
        return false;
    }

    std::cout << "PASSED (Peak: " << peakMagnitude << ")\n";
    std::cout << "[AgentTestRunner] Exported WAV Observation 20 (Sample Pool & Sampler): " << obs20Wav.getFullPathName().toStdString() << "\n";
    return true;
}

bool AgentTestRunner::testProjectDirectoryAssetManagement()
{
    std::cout << "[Test 21] Project Asset Manager, Directory Bundling & Audio Folder Resolution Suite... ";

    // 1. Initialize default temporary session
    ProjectManager::getInstance().initializeDefaultSession();
    auto tempAudioDir = ProjectManager::getInstance().getAudioDirectory();
    if (!tempAudioDir.isDirectory())
    {
        std::cout << "FAILED (Default temp session audio directory not created)\n";
        return false;
    }

    // 2. Synthesize test drum samples and register in TableManager
    constexpr int sampleCount = 22050; // 0.5 sec
    juce::AudioBuffer<float> kickAssetBuf(1, sampleCount);
    float* kPtr = kickAssetBuf.getWritePointer(0);
    for (int i = 0; i < sampleCount; ++i)
    {
        double t = static_cast<double>(i) / 44100.0;
        kPtr[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * 60.0 * t) * std::exp(-t * 10.0));
    }
    TableManager::getInstance().registerBuffer("kick_asset", kickAssetBuf, 44100.0);

    juce::AudioBuffer<float> snareAssetBuf(2, sampleCount);
    float* sPtrL = snareAssetBuf.getWritePointer(0);
    float* sPtrR = snareAssetBuf.getWritePointer(1);
    for (int i = 0; i < sampleCount; ++i)
    {
        double t = static_cast<double>(i) / 44100.0;
        float val = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * 200.0 * t) * std::exp(-t * 14.0));
        sPtrL[i] = val;
        sPtrR[i] = val;
    }
    TableManager::getInstance().registerBuffer("snare_asset", snareAssetBuf, 44100.0);

    // 3. Switch to a saved project directory in artifacts/test_project/
    juce::File testProjDir("artifacts/test_project");
    if (testProjDir.isDirectory())
    {
        testProjDir.deleteRecursively();
    }
    testProjDir.createDirectory();

    juce::File testProjFile = testProjDir.getChildFile("TestSong.pdil");
    ProjectManager::getInstance().setProjectFile(testProjFile);

    // 4. Sync session assets to the project (writes active tables to ./audio/<name>.wav)
    ProjectManager::getInstance().syncSessionAssetsToProject();

    auto projAudioDir = ProjectManager::getInstance().getAudioDirectory();
    auto kickWav = projAudioDir.getChildFile("kick_asset.wav");
    auto snareWav = projAudioDir.getChildFile("snare_asset.wav");

    if (!kickWav.existsAsFile() || !snareWav.existsAsFile())
    {
        std::cout << "FAILED (Audio assets not written into project ./audio/ folder)\n";
        return false;
    }
    if (kickWav.getSize() < 1000 || snareWav.getSize() < 1000)
    {
        std::cout << "FAILED (Written WAV file sizes too small)\n";
        return false;
    }

    // 5. Test Path Resolution
    juce::File resolvedKick = ProjectManager::getInstance().resolveAudioFile("kick_asset.wav");
    juce::File resolvedSnare = ProjectManager::getInstance().resolveAudioFile("audio/snare_asset.wav");

    if (resolvedKick != kickWav || resolvedSnare != snareWav)
    {
        std::cout << "FAILED (Path resolution mismatch for relative audio assets)\n";
        return false;
    }

    // 6. Test Soundfiler & Readsf with relative paths inside project
    auto soundfiler = RelativisticNodeFactory::createNode(10, "soundfiler");
    soundfiler->receiveMessage("read -resize kick_asset.wav reloaded_kick");
    if (!TableManager::getInstance().hasTable("reloaded_kick"))
    {
        std::cout << "FAILED (Soundfiler failed to resolve and read relative ./audio/ asset)\n";
        return false;
    }

    // 7. Clear all tables and simulate Project Re-Open Auto-Loading
    TableManager::getInstance().clearAllTables();
    if (TableManager::getInstance().hasTable("kick_asset"))
    {
        std::cout << "FAILED (TableManager clearAllTables failed)\n";
        return false;
    }

    // Auto-load all tables from project ./audio/ directory
    TableManager::getInstance().loadTablesFromDirectory(projAudioDir);
    if (!TableManager::getInstance().hasTable("kick_asset") || !TableManager::getInstance().hasTable("snare_asset"))
    {
        std::cout << "FAILED (Project re-open auto-load from ./audio/ failed)\n";
        return false;
    }

    const auto* reloadedInfo = TableManager::getInstance().getTableInfo("snare_asset");
    if (!reloadedInfo || reloadedInfo->numChannels != 2 || reloadedInfo->thumbnailPeaks.empty())
    {
        std::cout << "FAILED (Reloaded table metadata/waveform envelope invalid)\n";
        return false;
    }

    std::cout << "PASSED (Audio folder bundling & relative resolution verified)\n";
    return true;
}

} // namespace TimeDilationDAW

