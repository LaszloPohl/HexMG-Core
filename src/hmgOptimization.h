#pragma once
#include <atomic>
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
        } else fail();
    }
    precision = next; precisionSeen = seenPrecision; threadsSeen = seenThreads;
}
inline void report() {
    std::cout << "HEXMG_OPT: LEVEL0 " << (precision == Precision::Extended ? "EXTENDED" : precision == Precision::Double ? "DOUBLE" : "AUTO")
        << "; reference-checked double steps=" << doubleSteps.load()
        << "; extended fallbacks=" << extendedFallbacks.load()
        << "; higher levels unchanged; AC unchanged\n";
}
}
