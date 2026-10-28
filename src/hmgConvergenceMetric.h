#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nsHMG::convergence {
// Arithmetic foundation only; not wired into Simulation yet.
// Callers supply physical scales and tolerances explicitly: no implicit units.
struct Tolerances { double relative, stateAbsolute, balanceAbsolute; };
inline double normalized(double error, double reference, double absolute, double relative) {
    if (!std::isfinite(error) || !std::isfinite(reference) || reference < 0 ||
        !std::isfinite(absolute) || absolute <= 0 || !std::isfinite(relative) || relative <= 0)
        throw std::invalid_argument("Invalid convergence input or tolerance");
    const double limit = std::fma(relative, reference, absolute);
    if (!std::isfinite(limit) || limit <= 0)
        throw std::invalid_argument("Unrepresentable convergence limit");
    // Infinity from division is a failed convergence test, never success.
    return std::abs(error) / limit;
}
inline double nodeError(double actualChange, double stateReference,
                        double physicalBalance, double absoluteBranchSum,
                        Tolerances tolerance) {
    return std::max(normalized(actualChange, stateReference, tolerance.stateAbsolute, tolerance.relative),
                    normalized(physicalBalance, absoluteBranchSum, tolerance.balanceAbsolute, tolerance.relative));
}
class Collector {
    double maximum_ = 0;
public:
    void add(double error) {
        if (std::isnan(error) || error < 0) throw std::invalid_argument("Invalid normalized convergence error");
        maximum_ = std::max(maximum_, error);
    }
    double maximum() const { return maximum_; }
    bool converged() const { return maximum_ <= 1; }
};
}
