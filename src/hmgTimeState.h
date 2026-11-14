#pragma once
#include "hmgCircuitNode.hpp"
#include <algorithm>
#include <vector>

namespace nsHMG {
// A timestep transaction owns only numeric state, never topology or matrix
// storage. Jacobians/residuals are rebuilt by CalculateValuesAndCurrentsDC.
// Collect once per RUN; refresh the checkpoint only after an accepted step.
class TimeState {
    struct NodeState { NodeVariable* node; rvt value{}, previous{}; cplx correction{}; };
    struct FieldState { std::vector<rvt>* field; std::vector<rvt> values; };
    struct ComplexFieldState { std::vector<cplx>* field; std::vector<cplx> values; };
    std::vector<ComplexFieldState> complexFields;
    struct ScalarState { rvt* field; rvt value{}; };
    std::vector<NodeState> nodes;
    std::vector<FieldState> fields;
    std::vector<ScalarState> scalars;
    rvt alpha=1;
public:
    void add(NodeVariable* node) { if(node) nodes.push_back({node}); }
    void add(rvt& field) { scalars.push_back({&field}); }
    void add(std::vector<rvt>& field) { fields.push_back({&field,{}}); }
    void add(std::vector<cplx>& field) { complexFields.push_back({&field,{}}); }
    void prepare() {
        const auto less=[](const auto& a,const auto& b){return std::less<NodeVariable*>{}(a.node,b.node);};
        std::sort(nodes.begin(),nodes.end(),less);
        nodes.erase(std::unique(nodes.begin(),nodes.end(),[](const auto& a,const auto& b){return a.node==b.node;}),nodes.end());
        capture();
    }
    void capture() {
        alpha=NodeVariable::alpha;
        for(auto& state:nodes) {
            state.value=state.node->valueDC;state.previous=state.node->stepStartDC;
            state.correction=state.node->extension?state.node->extension->v:cplx0;
        }
        for(auto& state:fields) state.values=*state.field;
        for(auto& state:scalars) state.value=*state.field;
        for(auto& state:complexFields) state.values=*state.field;
    }
    void requireFinite() const {
        for(const auto& state:nodes)if(!std::isfinite(state.node->getValueDC())||!std::isfinite(state.node->stepStartDC))
            throw hmgExcept("ADAPTIVE_VOLTAGE","Non-finite timestep state");
    }
    void restore() noexcept {
        NodeVariable::alpha=alpha;
        for(const auto& state:nodes) {
            state.node->valueDC=state.value;state.node->stepStartDC=state.previous;
            if(auto* ext=state.node->extension.get()) {
                ext->v=state.correction;
                ext->d.store(cplx0,state.node->isConcurrent);
                ext->physicalMagnitude.store(cplx0,state.node->isConcurrent);
                ext->yii.store(cplx0,state.node->isConcurrent);ext->f=cplx0;
            }
        }
        for(const auto& state:fields) std::copy(state.values.begin(),state.values.end(),state.field->begin());
        for(const auto& state:scalars) *state.field=state.value;
        for(const auto& state:complexFields) std::copy(state.values.begin(),state.values.end(),state.field->begin());
    }
};
}
