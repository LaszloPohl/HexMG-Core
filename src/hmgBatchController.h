#pragma once
#include "hmgCommon.h"
#include "hmgException.h"
#include "hmgOpticalRayFile.h"
#include <functional>
#include <memory>

namespace nsHMG {
std::size_t opticalAttenuationCacheBudget() noexcept;
inline std::size_t opticalAttenuationCacheBudget(const hexmg::optics::File& file) noexcept {
    std::size_t fixed=0,varying=0;
    for(const auto& cell:file.cells){const auto& blue=file.bindings[cell.coefficients[0]];const auto& yellow=file.bindings[cell.coefficients[1]];
        if(!blue.address.empty()||!yellow.address.empty())++varying;
        else if(blue.constant>0||yellow.constant>0)++fixed;
    }
    // A cache is useful for fixed absorption. Rewriting a multi-GiB cache
    // every temperature iteration can be slower than direct evaluation.
    return fixed&&fixed>=varying?opticalAttenuationCacheBudget():0;
}
// Common controller lifecycle. A type supplies only the bulk calculation;
// circuit reads precede every controller store at the same level.
class BatchController {
public:
    std::string name;uns level=0;
    std::vector<DeepInterfaceNodeID> nodes;
    virtual ~BatchController()=default;
    virtual void load(const std::function<double(const DeepInterfaceNodeID&)>&)=0;
    virtual void evaluateAndStore(const std::function<void(const DeepInterfaceNodeID&,double)>&)=0;
};
class OpticalBatchController final:public BatchController {
    std::shared_ptr<const hexmg::optics::File> file_;
    hexmg::optics::Executor<double> executor_;
    std::vector<double> values_,blue_,yellow_;
    std::vector<std::array<double,4>> coefficients_;
public:
    OpticalBatchController(std::shared_ptr<const hexmg::optics::File> file,std::vector<DeepInterfaceNodeID> ids)
        :file_(std::move(file)),executor_(file_->program,file_->blue.size(),file_->yellow.size(),opticalAttenuationCacheBudget(*file_),std::max(1u,std::min(8u,std::thread::hardware_concurrency()/2))),
        values_(file_->bindings.size()),blue_(file_->blue.size()),yellow_(file_->yellow.size()),coefficients_(file_->cells.size()) {nodes=std::move(ids);}
    void load(const std::function<double(const DeepInterfaceNodeID&)>& read)override{
        for(std::size_t i=0;i<values_.size();++i)values_[i]=file_->bindings[i].address.empty()?file_->bindings[i].constant:read(nodes[i]);
        for(std::size_t i=0;i<coefficients_.size();++i)for(unsigned k=0;k<4;++k)coefficients_[i][k]=values_[file_->cells[i].coefficients[k]];
        for(std::size_t i=0;i<blue_.size();++i)blue_[i]=values_[file_->blue[i]];
        for(std::size_t i=0;i<yellow_.size();++i)yellow_[i]=values_[file_->yellow[i]];
    }
    void evaluateAndStore(const std::function<void(const DeepInterfaceNodeID&,double)>& write)override try {
        const auto& o=executor_.evaluate(coefficients_,blue_,yellow_);
        const auto store=[&](std::uint32_t id,double value){if(id!=hexmg::optics::noBinding)write(nodes[id],value);};
        for(std::size_t i=0;i<coefficients_.size();++i){store(file_->cells[i].outputs[0],o.heat[i]);store(file_->cells[i].outputs[1],o.wall[i]);store(file_->cells[i].outputs[2],o.yellow[i]);}
        for(std::size_t i=0;i<coefficients_.size();++i)for(unsigned k=0;k<2;++k)if(file_->cells[i].outputs[3+k]!=hexmg::optics::noBinding){
            const auto& values=k?o.yellowImage:o.blueImage;store(file_->cells[i].outputs[3+k],values.empty()?0:values[i]);}
        for(unsigned side=0;side<6;++side)for(unsigned band=0;band<2;++band)store(file_->exits[2*side+band],o.escape[side][band]);
        for(unsigned band=0;band<2;++band)store(file_->exits[12+band],o.cutoff[band]);
    } catch(const hmgExcept&) { throw; }
      catch(const std::exception& error) { throw hmgExcept("BATCHCONTROLLER","%.200s: %.500s",name.c_str(),error.what()); }
};
}
