#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>
#include <random>

namespace TimeDilationDAW
{

struct TidalEvent
{
    double startCycle = 0.0; // 0.0 to 1.0 within cycle
    double endCycle = 1.0;
    int pitch = 60;          // MIDI pitch or 0 for rests
    float velocity = 0.85f;
    int channel = 0;         // Parallel stacked voice channel (0, 1, 2...)
    bool isRest = false;
    std::string valueStr;    // E.g. "60", "bd", "cp", "hh"
};

class TidalPattern
{
public:
    virtual ~TidalPattern() = default;
    virtual void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const = 0;
};

class TidalAtom : public TidalPattern
{
public:
    TidalAtom(const std::string& val, double prob = 1.0f, double speedMult = 1.0f);
    void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const override;

private:
    std::string value;
    int pitch = 60;
    bool isRest = false;
    double probability = 1.0;
    double speedMultiplier = 1.0;
};

class TidalSequence : public TidalPattern
{
public:
    TidalSequence(std::vector<std::shared_ptr<TidalPattern>> elements);
    void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const override;

private:
    std::vector<std::shared_ptr<TidalPattern>> elements;
};

class TidalStack : public TidalPattern
{
public:
    TidalStack(std::vector<std::shared_ptr<TidalPattern>> parallelPatterns);
    void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const override;

private:
    std::vector<std::shared_ptr<TidalPattern>> parallelPatterns;
};

class TidalAlternation : public TidalPattern
{
public:
    TidalAlternation(std::vector<std::shared_ptr<TidalPattern>> cycleChoices);
    void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const override;

private:
    std::vector<std::shared_ptr<TidalPattern>> choices;
};

class TidalEuclidean : public TidalPattern
{
public:
    TidalEuclidean(std::shared_ptr<TidalPattern> pattern, int pulses, int steps, int rotation = 0);
    void query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel = 0) const override;

private:
    std::shared_ptr<TidalPattern> subPattern;
    int pulses = 3;
    int steps = 8;
    int rotation = 0;
    std::vector<bool> bjorklund;
};

// Parser for TidalCycles mini-notation strings
class TidalParser
{
public:
    static std::shared_ptr<TidalPattern> parse(const std::string& input);

private:
    static std::shared_ptr<TidalPattern> parseSequence(const std::string& input, size_t& pos);
    static std::shared_ptr<TidalPattern> parseAtom(const std::string& input, size_t& pos);
    static void skipWhitespace(const std::string& input, size_t& pos);
};

} // namespace TimeDilationDAW
