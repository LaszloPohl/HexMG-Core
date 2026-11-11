#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include "miniz.h"
namespace nsHMG {
// HXHB0001: eight-byte magic, little-endian uint64 decoded size, zlib stream.
// Byte-exact HMG payload; existing lexer, INCLUDE paths and line diagnostics remain authoritative.
inline bool readBinaryHmg(FILE* file,std::vector<char>& decoded) {
    char header[16]{};const auto count=fread(header,1,16,file);
    if(count<4||memcmp(header,"HXHB",4)!=0){rewind(file);return false;}
    if(count!=16||memcmp(header,"HXHB0001",8)!=0)throw std::runtime_error("Unsupported or truncated binary HMG header");
    uint64_t size=0;for(unsigned i=0;i<8;++i)size|=uint64_t(static_cast<unsigned char>(header[8+i]))<<(8*i);
    if(size==0||size>uint64_t(2)*1024*1024*1024)throw std::runtime_error("Invalid binary HMG decoded size (maximum 2 GiB)");
    std::vector<unsigned char> packed;unsigned char buffer[65536];size_t n;
    while((n=fread(buffer,1,sizeof(buffer),file))!=0){if(packed.size()+n>size+size/100+65536)throw std::runtime_error("Binary HMG input too large");packed.insert(packed.end(),buffer,buffer+n);}
    if(ferror(file))throw std::runtime_error("Cannot read binary HMG");
    mz_stream stream{};stream.next_in=packed.data();stream.avail_in=unsigned(packed.size());
    if(mz_inflateInit(&stream)!=MZ_OK)throw std::runtime_error("Cannot initialize binary HMG decoder");
    int status=MZ_OK;
    try {
        while(status==MZ_OK){
            stream.next_out=buffer;stream.avail_out=sizeof(buffer);status=mz_inflate(&stream,MZ_NO_FLUSH);
            const auto produced=sizeof(buffer)-stream.avail_out;
            if(decoded.size()+produced>size)throw std::runtime_error("Binary HMG decoded size mismatch");
            decoded.insert(decoded.end(),buffer,buffer+produced);
        }
        if(status!=MZ_STREAM_END||decoded.size()!=size||stream.total_in!=packed.size())throw std::runtime_error("Damaged binary HMG payload");
        mz_inflateEnd(&stream);
    }catch(...){mz_inflateEnd(&stream);throw;}
    return true;
}
}
