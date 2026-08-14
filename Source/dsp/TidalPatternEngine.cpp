#include "TidalPatternEngine.h"
#include <sstream>
#include <cctype>

namespace TimeDilationDAW
{

// ==============================================================================
// TidalAtom Implementation
// ==============================================================================
TidalAtom::TidalAtom(const std::string& val, double prob, double speedMult)
    : value(val), probability(prob), speedMultiplier(speedMult)
{
    if (val == "~" || val == "-")
    {
        isRest = true;
        pitch = 0;
    }
    else
    {
        isRest = false;
        try {
            pitch = std::stoi(val);
        } catch (...) {
            // Check for drum names
            if (val == "bd" || val == "kick") pitch = 36;
            else if (val == "sn" || val == "snare") pitch = 38;
            else if (val == "hh" || val == "hat") pitch = 42;
            else if (val == "cp" || val == "clap") pitch = 39;
            else pitch = 60; // Fallback
        }
    }
}

void TidalAtom::query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel) const
{
    (void)cycleNumber;

    if (probability < 1.0)
    {
        static std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) > probability) return;
    }

    if (speedMultiplier <= 1.0)
    {
        TidalEvent ev;
        ev.startCycle = cycleStart;
        ev.endCycle = cycleEnd;
        ev.pitch = pitch;
        ev.velocity = 0.85f;
        ev.channel = channel;
        ev.isRest = isRest;
        ev.valueStr = value;
        outEvents.push_back(ev);
    }
    else
    {
        int subSteps = static_cast<int>(speedMultiplier);
        double stepDur = (cycleEnd - cycleStart) / subSteps;
        for (int i = 0; i < subSteps; ++i)
        {
            TidalEvent ev;
            ev.startCycle = cycleStart + i * stepDur;
            ev.endCycle = cycleStart + (i + 1) * stepDur;
            ev.pitch = pitch;
            ev.velocity = 0.85f;
            ev.channel = channel;
            ev.isRest = isRest;
            ev.valueStr = value;
            outEvents.push_back(ev);
        }
    }
}

// ==============================================================================
// TidalSequence Implementation
// ==============================================================================
TidalSequence::TidalSequence(std::vector<std::shared_ptr<TidalPattern>> elms)
    : elements(std::move(elms))
{
}

void TidalSequence::query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel) const
{
    if (elements.empty()) return;

    double totalSpan = cycleEnd - cycleStart;
    double stepSpan = totalSpan / elements.size();

    for (size_t i = 0; i < elements.size(); ++i)
    {
        double subStart = cycleStart + i * stepSpan;
        double subEnd = subStart + stepSpan;
        if (elements[i])
        {
            elements[i]->query(subStart, subEnd, cycleNumber, outEvents, channel);
        }
    }
}

// ==============================================================================
// TidalStack Implementation
// ==============================================================================
TidalStack::TidalStack(std::vector<std::shared_ptr<TidalPattern>> parallel)
    : parallelPatterns(std::move(parallel))
{
}

void TidalStack::query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel) const
{
    for (size_t i = 0; i < parallelPatterns.size(); ++i)
    {
        if (parallelPatterns[i])
        {
            parallelPatterns[i]->query(cycleStart, cycleEnd, cycleNumber, outEvents, channel + static_cast<int>(i));
        }
    }
}

// ==============================================================================
// TidalAlternation Implementation
// ==============================================================================
TidalAlternation::TidalAlternation(std::vector<std::shared_ptr<TidalPattern>> ch)
    : choices(std::move(ch))
{
}

void TidalAlternation::query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel) const
{
    if (choices.empty()) return;
    size_t idx = static_cast<size_t>((cycleNumber % static_cast<int>(choices.size()) + static_cast<int>(choices.size())) % static_cast<int>(choices.size()));
    if (choices[idx])
    {
        choices[idx]->query(cycleStart, cycleEnd, cycleNumber, outEvents, channel);
    }
}

// ==============================================================================
// TidalEuclidean Implementation
// ==============================================================================
TidalEuclidean::TidalEuclidean(std::shared_ptr<TidalPattern> pattern, int k, int n, int rot)
    : subPattern(pattern), pulses(k), steps(n), rotation(rot)
{
    if (steps <= 0) steps = 8;
    int actualK = std::clamp(pulses, 0, steps);

    bjorklund.assign(static_cast<size_t>(steps), false);
    if (actualK > 0)
    {
        std::vector<std::vector<bool>> groups;
        for (int i = 0; i < steps; ++i) groups.push_back({ i < actualK });

        int countFalse = steps - actualK;
        int countTrue = actualK;

        while (countFalse > 0)
        {
            int minCount = std::min(countTrue, countFalse);
            for (int i = 0; i < minCount; ++i)
            {
                auto& backGroup = groups.back();
                groups[static_cast<size_t>(i)].insert(groups[static_cast<size_t>(i)].end(), backGroup.begin(), backGroup.end());
                groups.pop_back();
            }
            if (countTrue > countFalse) countTrue -= countFalse;
            else countFalse -= countTrue;
        }

        bjorklund.clear();
        for (const auto& g : groups) bjorklund.insert(bjorklund.end(), g.begin(), g.end());

        if (!bjorklund.empty() && rotation != 0)
        {
            int r = (rotation % static_cast<int>(bjorklund.size()) + static_cast<int>(bjorklund.size())) % static_cast<int>(bjorklund.size());
            std::rotate(bjorklund.begin(), bjorklund.begin() + r, bjorklund.end());
        }
    }
}

void TidalEuclidean::query(double cycleStart, double cycleEnd, int cycleNumber, std::vector<TidalEvent>& outEvents, int channel) const
{
    if (steps <= 0 || !subPattern) return;
    double totalSpan = cycleEnd - cycleStart;
    double stepSpan = totalSpan / steps;

    for (int s = 0; s < steps; ++s)
    {
        if (!bjorklund.empty() && bjorklund[static_cast<size_t>(s)])
        {
            double subStart = cycleStart + s * stepSpan;
            double subEnd = subStart + stepSpan;
            subPattern->query(subStart, subEnd, cycleNumber, outEvents, channel);
        }
    }
}

// ==============================================================================
// TidalParser Implementation
// ==============================================================================
void TidalParser::skipWhitespace(const std::string& input, size_t& pos)
{
    while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos])))
    {
        pos++;
    }
}

std::shared_ptr<TidalPattern> TidalParser::parse(const std::string& input)
{
    size_t pos = 0;
    skipWhitespace(input, pos);
    if (pos >= input.size()) return std::make_shared<TidalAtom>("~");

    return parseSequence(input, pos);
}

std::shared_ptr<TidalPattern> TidalParser::parseSequence(const std::string& input, size_t& pos)
{
    std::vector<std::shared_ptr<TidalPattern>> currentLane;
    std::vector<std::shared_ptr<TidalPattern>> stackedLanes;

    while (pos < input.size())
    {
        skipWhitespace(input, pos);
        if (pos >= input.size()) break;

        char c = input[pos];
        if (c == ']' || c == '>')
        {
            break;
        }
        else if (c == ',')
        {
            // End current parallel lane in a stack
            if (!currentLane.empty())
            {
                if (currentLane.size() == 1) stackedLanes.push_back(currentLane.front());
                else stackedLanes.push_back(std::make_shared<TidalSequence>(currentLane));
                currentLane.clear();
            }
            pos++;
            continue;
        }
        else if (c == '[')
        {
            pos++; // Skip '['
            auto nested = parseSequence(input, pos);
            if (pos < input.size() && input[pos] == ']') pos++; // Skip ']'

            // Check for speed multiplier or Euclidean notation on the nested block (e.g. [60 62]*2 or [60 64](3,8))
            skipWhitespace(input, pos);
            if (pos < input.size() && input[pos] == '*')
            {
                pos++;
                size_t numStart = pos;
                while (pos < input.size() && (std::isdigit(static_cast<unsigned char>(input[pos])) || input[pos] == '.')) pos++;
                double mult = 2.0;
                try { mult = std::stod(input.substr(numStart, pos - numStart)); } catch (...) {}
                std::vector<std::shared_ptr<TidalPattern>> multRepeats;
                for (int r = 0; r < static_cast<int>(mult); ++r) multRepeats.push_back(nested);
                currentLane.push_back(std::make_shared<TidalSequence>(multRepeats));
            }
            else if (pos < input.size() && input[pos] == '(')
            {
                pos++;
                int k = 3, n = 8, rot = 0;
                size_t endParen = input.find(')', pos);
                if (endParen != std::string::npos)
                {
                    std::string inner = input.substr(pos, endParen - pos);
                    std::replace(inner.begin(), inner.end(), ',', ' ');
                    std::istringstream iss(inner);
                    iss >> k >> n >> rot;
                    pos = endParen + 1;
                    currentLane.push_back(std::make_shared<TidalEuclidean>(nested, k, n, rot));
                }
                else
                {
                    currentLane.push_back(nested);
                }
            }
            else
            {
                currentLane.push_back(nested);
            }
        }
        else if (c == '<')
        {
            pos++; // Skip '<'
            std::vector<std::shared_ptr<TidalPattern>> choices;
            while (pos < input.size() && input[pos] != '>')
            {
                skipWhitespace(input, pos);
                if (pos >= input.size() || input[pos] == '>') break;
                choices.push_back(parseAtom(input, pos));
            }
            if (pos < input.size() && input[pos] == '>') pos++; // Skip '>'
            currentLane.push_back(std::make_shared<TidalAlternation>(choices));
        }
        else
        {
            currentLane.push_back(parseAtom(input, pos));
        }
    }

    if (!stackedLanes.empty())
    {
        if (!currentLane.empty())
        {
            if (currentLane.size() == 1) stackedLanes.push_back(currentLane.front());
            else stackedLanes.push_back(std::make_shared<TidalSequence>(currentLane));
        }
        return std::make_shared<TidalStack>(stackedLanes);
    }

    if (currentLane.empty()) return std::make_shared<TidalAtom>("~");
    if (currentLane.size() == 1) return currentLane.front();
    return std::make_shared<TidalSequence>(currentLane);
}

std::shared_ptr<TidalPattern> TidalParser::parseAtom(const std::string& input, size_t& pos)
{
    skipWhitespace(input, pos);
    size_t start = pos;

    while (pos < input.size() && !std::isspace(static_cast<unsigned char>(input[pos])) &&
           input[pos] != '[' && input[pos] != ']' && input[pos] != '<' && input[pos] != '>' &&
           input[pos] != ',' && input[pos] != '(' && input[pos] != '*' && input[pos] != '?')
    {
        pos++;
    }

    std::string token = input.substr(start, pos - start);
    double prob = 1.0;
    double speedMult = 1.0;

    // Check for probability '?'
    if (pos < input.size() && input[pos] == '?')
    {
        pos++;
        size_t pStart = pos;
        while (pos < input.size() && (std::isdigit(static_cast<unsigned char>(input[pos])) || input[pos] == '.')) pos++;
        if (pos > pStart)
        {
            try { prob = std::stod(input.substr(pStart, pos - pStart)); } catch (...) { prob = 0.5; }
        }
        else
        {
            prob = 0.5;
        }
    }

    // Check for speed multiplier '*'
    if (pos < input.size() && input[pos] == '*')
    {
        pos++;
        size_t mStart = pos;
        while (pos < input.size() && (std::isdigit(static_cast<unsigned char>(input[pos])) || input[pos] == '.')) pos++;
        if (pos > mStart)
        {
            try { speedMult = std::stod(input.substr(mStart, pos - mStart)); } catch (...) { speedMult = 2.0; }
        }
        else
        {
            speedMult = 2.0;
        }
    }

    // Check for Euclidean brackets '(k, n, rot)'
    if (pos < input.size() && input[pos] == '(')
    {
        pos++;
        int k = 3, n = 8, rot = 0;
        size_t endParen = input.find(')', pos);
        if (endParen != std::string::npos)
        {
            std::string inner = input.substr(pos, endParen - pos);
            std::replace(inner.begin(), inner.end(), ',', ' ');
            std::istringstream iss(inner);
            iss >> k >> n >> rot;
            pos = endParen + 1;
            auto atom = std::make_shared<TidalAtom>(token, prob, speedMult);
            return std::make_shared<TidalEuclidean>(atom, k, n, rot);
        }
    }

    return std::make_shared<TidalAtom>(token, prob, speedMult);
}

} // namespace TimeDilationDAW
