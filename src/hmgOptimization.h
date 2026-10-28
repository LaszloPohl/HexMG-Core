#pragma once
#include <atomic>
#include <charconv>
#include <cmath>
#include <set>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "hmgCommon.h"
#include "hmgException.h"

namespace nsHMG::optimization {
enum class Precision { Extended, Double, Auto };
inline bool enabled = false; // Explicitly enabled by the optimized executable.
inline Precision precision = Precision::Extended;
inline bool precisionSeen = false;
inline bool threadsSeen = false;
inline unsigned maxIterations = 0; // Zero preserves legacy full-solve bounds.
inline bool maxIterationsSeen = false;
struct ConvergenceProfile { double reference=0, stateAbsolute=1e-6, balanceAbsolute=1e-12; };
inline bool relativeConvergence = false;
inline ConvergenceProfile convergenceDefault;
struct NodeConvergenceOption {
    std::string circuit, node;
    ConvergenceProfile profile;
    unsigned circuitId = 0;
    DeepInterfaceNodeID id;
};
inline std::vector<NodeConvergenceOption> convergenceNodes;
inline bool unsupportedWarned = false;
inline std::atomic<unsigned long long> doubleSteps{0}, extendedFallbacks{0};
inline std::vector<std::string> tokens(std::string line) {
    const auto semicolon = line.find(';'), slash = line.find("//");
    line.resize(std::min(line.size(), std::min(semicolon, slash)));
    for (char& c : line) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    std::istringstream stream(line); std::vector<std::string> result; std::string token;
    while (stream >> token) result.push_back(token);
    return result;
}
inline bool knownHeader(const std::string& header, const std::string& file, unsigned line) {
    if (!enabled) return false;
    const auto t = tokens(header);
    if(t.size()>=3 && t[1]=="HEXMG_OPT" && t[2]=="1" && t.size()!=3)
        throw hmgExcept("HEXMG_OPT","invalid version-1 header in %s, line %u",file.c_str(),line);
    const bool known = t == std::vector<std::string>{".OPTIONAL", "HEXMG_OPT", "1"};
    if (!known && !unsupportedWarned) { unsupportedWarned=true;std::cerr << "HEXMG_OPT: unsupported OPTIONAL header; block ignored\n"; }
    return known;
}
inline void parse(const std::vector<std::string>& lines, const std::string& file, unsigned openingLine) {
    unsigned nextMax = maxIterations; bool seenMax = maxIterationsSeen;
    bool nextRelative = relativeConvergence;
    auto nextProfile = convergenceDefault;
    auto nextNodes = convergenceNodes;
    std::set<std::pair<std::string,std::string>> nodeKeys;
    for(const auto& node:nextNodes) nodeKeys.emplace(node.circuit,node.node);
    auto next = precision; bool seenPrecision = precisionSeen, seenThreads = threadsSeen;
    for (unsigned i = 0; i < lines.size(); ++i) {
        const auto t = tokens(lines[i]); if (t.empty()) continue;
        const auto fail = [&] { throw hmgExcept("HEXMG_OPT", "invalid or repeated option in %s, line %u", file.c_str(), openingLine + i + 1); };
        if (t.size() == 3 && t[0] == "REDUCTION_PRECISION" && t[1] == "LEVEL0") {
            if (seenPrecision) fail();
            if (t[2] == "EXTENDED") next = Precision::Extended;
            else if (t[2] == "DOUBLE") next = Precision::Double;
            else if (t[2] == "AUTO") next = Precision::Auto;
            else fail();
            seenPrecision = true;
        } else if (t.size() == 2 && t[0] == "THREADS" && (t[1] == "AUTO" || t[1] == "1")) {
            if (seenThreads) fail();
            seenThreads = true;
            std::cout << "HEXMG_OPT: numerical execution is serial\n";
        } else if (t.size() == 2 && t[0] == "MAX_ITERATIONS") {
            if (seenMax) fail();
            const auto parsed = std::from_chars(t[1].data(), t[1].data() + t[1].size(), nextMax);
            if (parsed.ec != std::errc{} || parsed.ptr != t[1].data() + t[1].size() || nextMax == 0) fail();
            seenMax = true;
        } else if ((t.size() == 5 && t[0] == "CONVERGENCE" && t[1] == "RELATIVE") ||
                   (t.size() == 6 && t[0] == "CONVERGENCE_NODE")) {
            const bool global = t[0] == "CONVERGENCE";
            ConvergenceProfile profile;
            const auto value = [&](size_t at) {
                double v = 0;
                const auto parsed = std::from_chars(t[at].data(), t[at].data()+t[at].size(), v);
                if (parsed.ec != std::errc{} || parsed.ptr != t[at].data()+t[at].size() || !std::isfinite(v)) fail();
                return v;
            };
            const size_t first = global ? 2 : 3;
            profile.reference=value(first); profile.stateAbsolute=value(first+1); profile.balanceAbsolute=value(first+2);
            if (profile.stateAbsolute <= 0 || profile.balanceAbsolute <= 0) fail();
            if (global) { if(nextRelative) fail(); nextRelative=true; nextProfile=profile; }
            else {
                if(!nodeKeys.emplace(t[1],t[2]).second) fail();
                nextNodes.push_back({t[1],t[2],profile,0,{}});
            }
        } else fail();
    }
    relativeConvergence=nextRelative; convergenceDefault=nextProfile; convergenceNodes=std::move(nextNodes);
    maxIterations = nextMax; maxIterationsSeen = seenMax;
    precision = next; precisionSeen = seenPrecision; threadsSeen = seenThreads;
}
inline void report() {
    std::cout << "HEXMG_OPT: LEVEL0 " << (precision == Precision::Extended ? "EXTENDED" : precision == Precision::Double ? "DOUBLE" : "AUTO")
        << "; reference-checked double steps=" << doubleSteps.load()
        << "; extended fallbacks=" << extendedFallbacks.load()
        << "; higher levels unchanged; AC unchanged\n";
}
}
