#pragma once
#include "hmgOpticalRayProgram.h"
#include <bit>
#include <string>
#include <set>

namespace hexmg::optics {
inline constexpr std::uint32_t noBinding=UINT32_MAX;
struct Binding { std::string address; double constant=0; };
struct CellBindings { std::array<std::uint32_t,4> coefficients{}; std::array<std::uint32_t,5> outputs{noBinding,noBinding,noBinding,noBinding,noBinding}; };
struct File {
    Program program;
    std::vector<Binding> bindings;
    std::vector<CellBindings> cells;
    std::vector<std::uint32_t> blue,yellow;
    std::array<std::uint32_t,14> exits{};
    File(){exits.fill(noBinding);}
    void validate()const{
        if(cells.size()!=program.cells||bindings.size()>=noBinding)throw std::invalid_argument("Optical binding dimensions");
        program.validate(blue.size(),yellow.size());std::set<std::string> destinations;
        for(const auto& b:bindings)if(!std::isfinite(b.constant)||b.address.size()>4096||b.address.find('\0')!=std::string::npos)
            throw std::invalid_argument("Invalid optical binding");
        const auto input=[&](std::uint32_t id){if(id>=bindings.size())throw std::invalid_argument("Invalid optical input binding");};
        const auto output=[&](std::uint32_t id){if(id==noBinding)return;input(id);
            if(bindings[id].address.empty()||!destinations.insert(bindings[id].address).second)throw std::invalid_argument("Missing or duplicate optical output binding");};
        for(const auto& c:cells){for(auto id:c.coefficients)input(id);for(auto id:c.outputs)output(id);}
        for(auto id:blue)input(id);for(auto id:yellow)input(id);for(auto id:exits)output(id);
    }
};
namespace file_detail {
inline std::uint64_t checksum(std::span<const std::uint8_t> bytes){std::uint64_t h=14695981039346656037ULL;for(auto b:bytes){h^=b;h*=1099511628211ULL;}return h;}
struct Writer {
    std::vector<std::uint8_t> data;
    void u64(std::uint64_t v){for(unsigned i=0;i<8;++i)data.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
    void u32(std::uint32_t v){for(unsigned i=0;i<4;++i)data.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
    void real(double v){u64(std::bit_cast<std::uint64_t>(v));}
};
struct Reader {
    std::span<const std::uint8_t> data;std::size_t pos=0;
    void need(std::size_t n)const{if(n>data.size()-pos)throw std::invalid_argument("Truncated optical file");}
    std::uint64_t u64(){need(8);std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::uint64_t(data[pos++])<<(8*i);return v;}
    std::uint32_t u32(){need(4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(data[pos++])<<(8*i);return v;}
    double real(){return std::bit_cast<double>(u64());}
    std::size_t count(std::size_t minimumRecord){const auto n=u64();if(n>SIZE_MAX||n>(data.size()-pos)/minimumRecord)throw std::invalid_argument("Optical file count exceeds content");return static_cast<std::size_t>(n);}
};
}
inline std::vector<std::uint8_t> encode(const File& f){
    f.validate();const bool images=std::ranges::any_of(f.program.steps,[](const auto& s){return s.op==Op::Observe;})||std::ranges::any_of(f.cells,[](const auto& c){return c.outputs[3]!=noBinding||c.outputs[4]!=noBinding;});
    file_detail::Writer w;for(char c:std::string(images?"HXRAY002":"HXRAY001"))w.data.push_back(static_cast<std::uint8_t>(c));
    w.u64(f.program.cells);w.real(f.program.blueDistance);w.real(f.program.yellowDistance);w.u32(f.program.independentYellow?1:0);
    w.u64(f.bindings.size());for(const auto& b:f.bindings){w.u32(static_cast<std::uint32_t>(b.address.size()));for(char c:b.address)w.data.push_back(static_cast<std::uint8_t>(c));w.real(b.constant);}
    w.u64(f.cells.size());for(const auto& c:f.cells){for(auto id:c.coefficients)w.u32(id);for(unsigned k=0;k<(images?5u:3u);++k)w.u32(c.outputs[k]);}
    for(const auto* ids:{&f.blue,&f.yellow}){w.u64(ids->size());for(auto id:*ids)w.u32(id);}for(auto id:f.exits)w.u32(id);
    w.u64(f.program.rays.size());for(const auto& r:f.program.rays){w.u64(r.begin);w.u32(r.count);w.u32(r.source);w.u32(r.branches);w.u32(r.band);w.real(r.fraction);}
    w.u64(f.program.steps.size());for(const auto& s:f.program.steps){w.u32(static_cast<std::uint32_t>(s.op));w.u32(s.index);w.real(s.value);}
    w.u64(file_detail::checksum(w.data));return std::move(w.data);
}
inline File decode(std::span<const std::uint8_t> data){
    if(data.size()<16)throw std::invalid_argument("Unsupported optical file signature");
    const std::string signature(reinterpret_cast<const char*>(data.data()),8);const bool images=signature=="HXRAY002";
    if(!images&&signature!="HXRAY001")throw std::invalid_argument("Unsupported optical file signature");
    file_detail::Reader trailer{data.last(8)};
    if(trailer.u64()!=file_detail::checksum(data.first(data.size()-8)))throw std::invalid_argument("Optical file checksum mismatch");
    file_detail::Reader r{data.first(data.size()-8),8};File f;
    const auto cells=r.u64();if(!cells||cells>UINT32_MAX)throw std::invalid_argument("Invalid optical cell count");f.program.cells=static_cast<std::size_t>(cells);
    f.program.blueDistance=r.real();f.program.yellowDistance=r.real();const auto independent=r.u32();if(independent>1)throw std::invalid_argument("Invalid optical flags");f.program.independentYellow=independent!=0;
    f.bindings.resize(r.count(12));for(auto& b:f.bindings){const auto n=r.u32();if(n>4096)throw std::invalid_argument("Optical address too long");r.need(n);b.address.assign(reinterpret_cast<const char*>(r.data.data()+r.pos),n);r.pos+=n;b.constant=r.real();}
    f.cells.resize(r.count(images?36:28));for(auto& c:f.cells){for(auto& id:c.coefficients)id=r.u32();for(unsigned k=0;k<(images?5u:3u);++k)c.outputs[k]=r.u32();}
    for(auto* ids:{&f.blue,&f.yellow}){ids->resize(r.count(4));for(auto& id:*ids)id=r.u32();}for(auto& id:f.exits)id=r.u32();
    f.program.rays.resize(r.count(32));for(auto& ray:f.program.rays){ray.begin=r.u64();ray.count=r.u32();ray.source=r.u32();ray.branches=r.u32();ray.band=r.u32();ray.fraction=r.real();}
    f.program.steps.resize(r.count(16));for(auto& s:f.program.steps){s.op=static_cast<Op>(r.u32());s.index=r.u32();s.value=r.real();}
    if(r.pos!=r.data.size())throw std::invalid_argument("Unexpected optical file trailing data");f.validate();return f;
}
}
