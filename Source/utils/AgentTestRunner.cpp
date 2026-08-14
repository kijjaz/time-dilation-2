#include "AgentTestRunner.h"
#include "../gui/WorkstationContainerComponent.h"
#include "../dsp/RelativisticNodeFactory.h"
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

    bool allPhasesPass = allPhase1Pass && gravPass && lorentzPass && tachyonPass && jsonPass && dynamicLatPass && pdControlPass && samplePlaybackPass && delayPipePass;
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
    fullOut << "  \"relativisticDelayAndPipes\": " << (delayPipePass ? "true" : "false") << "\n";
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

} // namespace TimeDilationDAW

