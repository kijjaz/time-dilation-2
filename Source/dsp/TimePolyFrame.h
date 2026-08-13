#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstddef>

namespace TimeDilationDAW
{

struct TimePolyStream
{
    double gamma = 1.0;         // dTau / dt
    double tau = 0.0;           // Accumulated proper time in seconds
    double offsetCoupling = 1.0; // Coupling factor between Speed gamma and Offset tau (1.0 = Full Physics, 0.0 = Decoupled, 0.5 = Elastic Return)
    double phaseOffset = 0.0;   // Phase shift offset [0, 2pi)
    double amplitude = 1.0;     // Stream gain weight
    double age = 0.0;           // Age in seconds
    double duration = 1.0;      // Grain duration in seconds
    bool active = true;

    double getWindowEnvelope() const
    {
        if (!active || duration <= 0.0) return 0.0;
        double normAge = age / duration;
        if (normAge < 0.0 || normAge > 1.0) return 0.0;
        // Sine window envelope
        return amplitude * std::sin(3.14159265358979323846 * normAge);
    }

    void advance(double dt)
    {
        if (!active) return;
        // Integrated proper time offset with configurable coupling factor C:
        double effectiveRate = 1.0 + offsetCoupling * (gamma - 1.0);
        tau += effectiveRate * dt;
        age += std::abs(gamma) * dt;
        if (age >= duration)
        {
            active = false;
        }
    }
};

struct TimePolyFrame
{
    static constexpr size_t kMaxStreams = 1024;
    
    double masterGamma = 1.0;
    double masterTau = 0.0;
    std::vector<TimePolyStream> streams;

    TimePolyFrame()
    {
        streams.reserve(kMaxStreams);
        // Default single stream
        TimePolyStream defaultStream;
        defaultStream.gamma = 1.0;
        defaultStream.duration = 1e9; // indefinite
        streams.push_back(defaultStream);
    }

    void reset()
    {
        masterGamma = 1.0;
        masterTau = 0.0;
        streams.clear();
        TimePolyStream s;
        s.gamma = 1.0;
        s.duration = 1e9;
        streams.push_back(s);
    }

    void advanceFrame(double dt)
    {
        masterTau += masterGamma * dt;
        for (auto& s : streams)
        {
            s.advance(dt);
        }
    }

    size_t getActiveCount() const
    {
        size_t count = 0;
        for (const auto& s : streams)
        {
            if (s.active) count++;
        }
        return count;
    }
};

} // namespace TimeDilationDAW
