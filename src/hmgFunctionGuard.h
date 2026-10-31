#pragma once
#include <cmath>
#include <limits>
#include <optional>
#include <string>

namespace nsHMG {
class ComponentAndControllerBase;
namespace function_guard {
struct Rule {
    std::string name;
    unsigned parameter=0, variable=0, functionId=0;
    std::optional<double> minimum, maximum;
};
enum class RangeTarget { Parameter, Variable, Result };
struct RangeRule {
    std::string name, selector;
    RangeTarget target=RangeTarget::Result;
    unsigned index=0, functionId=0;
    std::optional<double> minimum, maximum;
};
struct RangeFailure {
    const RangeRule* rule=nullptr;
    const ComponentAndControllerBase* owner=nullptr;
    double value=0;
};
inline thread_local RangeFailure rangeFailure;
inline bool checkRange(const RangeRule& rule,const ComponentAndControllerBase* owner,double value) noexcept {
    const bool valid=std::isfinite(value)&&(!rule.minimum||value>=*rule.minimum)&&(!rule.maximum||value<=*rule.maximum);
    if(!valid&&!rangeFailure.rule)rangeFailure={&rule,owner,value};
    return valid;
}
struct Failure {
    const Rule* rule=nullptr;
    const ComponentAndControllerBase* owner=nullptr;
    double temperature=0, conductivity=0;
    const char* reason=nullptr;
};
// The solver is currently serial. Keep evaluation noexcept; report at the
// checked outer DC boundary, before reduction/acceptance or any SAVE.
inline thread_local Failure failure;
inline bool enabled=false;
inline thread_local unsigned derivativeDepth=0;
struct DerivativeScope {
    DerivativeScope() noexcept { ++derivativeDepth; }
    ~DerivativeScope() { --derivativeDepth; }
};
inline void record(const Rule* rule,const ComponentAndControllerBase* owner,
                   double t,double k,const char* reason) noexcept {
    if(!failure.reason)failure={rule,owner,t,k,reason};
}
inline void check(const Rule& rule,const ComponentAndControllerBase* owner,
                  double t,double k) noexcept {
    if(derivativeDepth)return; // Synthetic finite-difference sample, not state.
    if(!std::isfinite(t)||(rule.minimum&&t<*rule.minimum)||(rule.maximum&&t>*rule.maximum))
        record(&rule,owner,t,k,"temperature outside validity interval");
    else if(!std::isfinite(k)||k<=0)
        record(&rule,owner,t,k,"conductivity must be finite and positive");
}
} // namespace function_guard
} // namespace nsHMG
