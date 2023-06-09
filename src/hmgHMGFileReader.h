//***********************************************************************
// HexMG HMG File Converter Header
// Creation date:  2021. 06. 20.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HO_HMG_FILE_CONVERTER_HEADER
#define HO_HMG_FILE_CONVERTER_HEADER
//***********************************************************************


//***********************************************************************
#include <vector>
#include <list>
#include <string>
#include <map>
#include "hmgException.h"
#include "hmgNameToIndex.h"
#include "hmgCommon.h"
#include "hmgInstructionStream.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
struct LineInfo {
//***********************************************************************
    unsigned sourceIndex; // in the sourceFileNames; if the line is read from more than one file, the index of the first file
    unsigned firstLine, lastLine; // if appended line, the first and last line number is different. 
    LineInfo() :sourceIndex{ 0 }, firstLine{ 0 }, lastLine{ 0 } {}
};


//***********************************************************************
inline void filePathAndNameCreator(const std::string& startPath, const std::string& nameAndPath, std::string& path, std::string& name) {
// startPath is used if a relative path is given. A relative path must start with a .
//***********************************************************************
    if (nameAndPath.length() == 0) {
        name.clear();
        path.clear();
        return;
    }
    
    size_t slashPosition = nameAndPath.length() - 1;
    while (slashPosition > 0 && nameAndPath[slashPosition] != '/' and nameAndPath[slashPosition] != '\\')
        slashPosition--;
    
    if (slashPosition == 0 && nameAndPath[0] != '/' and nameAndPath[0] != '\\') { // no path
        path = startPath;
        name = nameAndPath;
        return;
    }

    // there is path

    name = nameAndPath.substr(slashPosition + 1);

    if (nameAndPath[0] != '.') { // absolute path
        path = nameAndPath.substr(0, slashPosition + 1);
        return;
    }

    // relative path

    if (nameAndPath[1] == '/' || nameAndPath[1] == '\\')
        path = startPath + nameAndPath.substr(2, slashPosition + 1); // ignores '.' and '/' or '\'
    else
        path = startPath + nameAndPath.substr(0, slashPosition + 1);

    return;
}


//***********************************************************************
inline bool spiceTextToRvt(const char* text, uns& position, rvt& value) {
//***********************************************************************

    if (sscanf_s(text + position, "%lg", &value) != 1)
        return false;

    if (text[position] == '+' || text[position] == '-')
        position++;

    while (text[position] == '.' || isdigit(text[position]))
        position++;

    if (text[position] == 'E') {
        position++;

        if (text[position] == '+' || text[position] == '-')
            position++;

        if (!isdigit(text[position]))
            return false;

        while (isdigit(text[position]))
            position++;
    }

    if (isalpha(text[position])) {
        switch (text[position]) {
            case 'T': value *= 1e12; break;
            case 'G': value *= 1e9; break;
            case 'M':
                if (text[position + 1] == 'E' && text[position + 2] == 'G')
                    value *= 1e6;
                else if (text[position + 1] == 'I' && text[position + 2] == 'L')
                    value *= 25.4e-6;
                else value *= 0.001;
                break;
            case 'K': value *= 1000; break;
            case 'U': value *= 1e-6; break;
            case 'N': value *= 1e-9; break;
            case 'P': value *= 1e-12; break;
            case 'F': value *= 1e-15; break;
        }
        while (isalpha(text[position]))
            position++;
    }

    return true;
}


//***********************************************************************
inline bool spiceTextToRvt(const char* text,rvt& value) {
//***********************************************************************
    uns pos = 0;
    return spiceTextToRvt(text, pos, value);
}


//***********************************************************************
inline bool textToSimpleNodeID(const char* text, uns& position, SimpleNodeID& result) {
//***********************************************************************
    switch (text[position]) {
        case 'C':
            if (text[position + 1] == 'I' || text[position + 2] == 'N') {
                result.type = nvtCIN;
                if (sscanf_s(&text[position + 3], "%u", &result.index) != 1)
                    return false;
            }
            else {
                result.type = nvtCInternal;
                if (sscanf_s(&text[position + 1], "%u", &result.index) != 1)
                    return false;
            }
            break;
        case 'F':
            if (text[position + 1] != 'W' || text[position + 2] != 'O' || text[position + 3] != 'U' || text[position + 4] != 'T')
                return false;
            result.type = nvtFWOUT;
            if (sscanf_s(&text[position + 5], "%u", &result.index) != 1)
                return false;
            break;
        case 'G':
            if (text[position + 1] == 'V' && text[position + 2] == 'A' && text[position + 3] == 'R') {
                result.type = nvtVarGlobal;
                if (sscanf_s(&text[position + 4], "%u", &result.index) != 1)
                    return false;
            }
            else if (text[position + 1] == 'N' && text[position + 2] == 'D') {
                result.type = nvtGND;
            }
            else
                return false;
            break;
        case 'I':
            if (text[position + 1] != 'N')
                return false;
            result.type = nvtIN;
            if (sscanf_s(&text[position + 2], "%u", &result.index) != 1)
                return false;
            break;
        case 'N':
            result.type = nvtNInternal;
            if (sscanf_s(&text[position + 1], "%u", &result.index) != 1)
                return false;
            break;
        case 'O':
            if (text[position + 1] != 'U' || text[position + 2] != 'T')
                return false;
            result.type = nvtOUT;
            if (sscanf_s(&text[position + 3], "%u", &result.index) != 1)
                return false;
            break;
        case 'P':
            result.type = nvtParam;
            if (sscanf_s(&text[position + 1], "%u", &result.index) != 1)
                return false;
            break;
        case 'R':
            result.type = nvtRail;
            if (sscanf_s(&text[position + 1], "%u", &result.index) != 1)
                return false;
            break;
        case 'X':
            result.type = nvtIO;
            if (sscanf_s(&text[position + 1], "%u", &result.index) != 1)
                return false;
            break;
        case 'V':
            if (text[position + 1] != 'A' || text[position + 2] != 'R')
                return false;
            result.type = nvtVarInternal;
            if (sscanf_s(&text[position + 3], "%u", &result.index) != 1)
                return false;
            break;
        default:
            return false;
    }
    while (isupper(text[position]))
        position++;
    while (isdigit(text[position]))
        position++;
    return true;
}


//***********************************************************************
inline bool textToSimpleNodeID(const char* text, SimpleNodeID& result) {
//***********************************************************************
    uns pos = 0;
    return textToSimpleNodeID(text, pos, result);
}


//***********************************************************************
class ReadALine {
//***********************************************************************
    bool is_open, is_EOF;
    unsigned actLine;
    ReadALine* pInclude;
    FILE* fp;
    unsigned fileNameIndex;
    struct FileNameAndPath {
        std::string path, name;
        FileNameAndPath() {}
        FileNameAndPath(const std::string& path, const std::string& name) : path{ path }, name{ name } {}
    };
    inline static std::vector<FileNameAndPath> names;
    struct BufferBlock {
        static const unsigned bufferSize = 524'288;
        char buffer[bufferSize] = { 0 };
        unsigned elementNum = 0, index = 0;
    };
    BufferBlock* buffer1 = nullptr;
    BufferBlock* buffer2 = nullptr;
    //***********************************************************************
    void readNextBlock() {
    //***********************************************************************
        if (buffer1->index == buffer1->elementNum) {
            std::swap(buffer1, buffer2);
            buffer2->elementNum = unsigned(fread(buffer2->buffer, sizeof(char), BufferBlock::bufferSize, fp));
            buffer1->index = 0;
            if (buffer1->elementNum == 0 && buffer2->elementNum == 0){                
                fClose();
                return;
            }
            if (buffer1->elementNum == 0) // initial reading
                readNextBlock();
        }
    }
    //***********************************************************************
    void fClose() {
    //***********************************************************************
        if (fp != nullptr)
            fclose(fp);
        fp = nullptr;
        is_open = false;
        is_EOF = true;
        buffer1->index = buffer1->elementNum = buffer2->index = buffer2->elementNum = 0;
    }
    //***********************************************************************
    int getChar() {
    //***********************************************************************
        if (buffer1->index == buffer1->elementNum) {
            fClose();
            return EOF;
        }
        char ret = buffer1->buffer[buffer1->index];
        buffer1->index++;
        if (buffer1->index == buffer1->elementNum)
            readNextBlock();
        return ret == '\'' ? getChar() : ret; // removes ' from the input
    }
    //***********************************************************************
    int peekChar()const {
    //***********************************************************************
        return (buffer1->index == buffer1->elementNum) ? EOF : buffer1->buffer[buffer1->index]; // mishandles ', but that can only occur in numerical literals where we don't use peek.
    }
    //***********************************************************************
    int peekChar2()const { // peeks the 2nd char // mishandles ', but that can only occur in numerical literals where we don't use peek.
    //***********************************************************************
        if (buffer1->index == buffer1->elementNum)
            return EOF;
        if (buffer1->index + 1 == buffer1->elementNum) {
            return (buffer2->elementNum > 0) ? buffer2->buffer[0] : EOF;
        }
        return buffer1->buffer[buffer1->index + 1];
    }
    //***********************************************************************
    void appendLine(char* result, unsigned resultSize, LineInfo& lineInfo);
    //***********************************************************************
public:
    ReadALine() :is_open{ false }, is_EOF{ true }, actLine{ 0 }, pInclude{ nullptr }, fp{ nullptr }, 
        buffer1{ new BufferBlock }, buffer2{ new BufferBlock }, fileNameIndex{ 0 } {}
    ~ReadALine() { delete pInclude; delete buffer1; delete buffer2; }
    bool open(const std::string& filePath, const std::string& fileName);
    bool isOpen()const { return is_open; }
    bool isEOF()const { return is_EOF; }
    bool getLine(char* result, unsigned resultSize, LineInfo& lineInfo, bool isContinue = false);
    const std::string& getFileName()const { return names[fileNameIndex].name; }
    const std::string& getFileName(const LineInfo& lineInfo)const { return names[lineInfo.sourceIndex].name; }
};


//***********************************************************************
struct HMGFileSunredTree;
struct HMGFileModelDescription;
struct HMGFileProbe;
struct HMGFileCreate;
//***********************************************************************


//***********************************************************************
struct GlobalHMGFileNames {
//***********************************************************************
    std::map<std::string, uns> modelNames, sunredTreeNames, varNames, functionNames, probeNames, fullCircuitNames;
    std::vector<HMGFileSunredTree*> sunredTreeData;
    std::vector<HMGFileModelDescription*> modelData;
    std::vector<HMGFileProbe*> probeData;
    std::vector<HMGFileCreate*> fullCircuitData;
    //***********************************************************************
    bool textToProbeNodeID(char* token, uns fullCircuitIndex, ProbeNodeID& dest);
    //***********************************************************************
};


//***********************************************************************
struct HMGFileListItem {
//***********************************************************************
    inline static GlobalHMGFileNames globalNames;
    inline static uns globalNRails = 0; // for checking
    virtual ~HMGFileListItem() {}
    virtual void toInstructionStream(InstructionStream& iStream) = 0;
};


//***********************************************************************
struct HMGFileComponentInstanceLine : HMGFileListItem {
//***********************************************************************
    uns instanceIndex = 0; // component index or controller index

    uns modelIndex = 0; // modelNames, BuiltInComponentTemplateType
    std::vector<SimpleNodeID> nodes;
    std::vector<ParameterInstance> params;
    bool isDefaultRail = false;
    bool isController = false;
    bool isBuiltIn = false;
    uns defaultValueRailIndex = 0;
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsComponentInstanceInstruction(instanceIndex, modelIndex, isDefaultRail, defaultValueRailIndex, isController, isBuiltIn, (uns)nodes.size(), (uns)params.size()));
        for(const auto& node : nodes)
            iStream.add(new IsNodeValueInstruction(node));
        for (const auto& param : params)
            iStream.add(new IsParameterValueInstruction(param));
        iStream.add(new IsEndDefInstruction(sitComponentInstance, instanceIndex));
    }
};


//***********************************************************************
struct HMGFileModelDescription: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    enum ModelType{ hfmtSubcircuit, hfmtController };
    std::string fullName;
    uns modelIndex = 0;
    ModelType modelType = hfmtSubcircuit;
    bool isReplacer = false;
    ExternalConnectionSizePack externalNs;
    InternalNodeVarSizePack internalNs;
    HMGFileModelDescription* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::vector< HMGFileComponentInstanceLine* > instanceList;
    std::map<std::string, uns> componentInstanceNameIndex;
    std::map<std::string, uns> controllerInstanceNameIndex;
    std::map<std::string, uns> instanceListIndex;
    std::vector<DefaultRailRange> defaults;
    SolutionType solutionType = stFullMatrix;
    uns solutionDescriptionIndex = 0; // for sunred and multigrid 
    //***********************************************************************

    //***********************************************************************
    void clear() { for (auto it : instanceList) delete it; instanceList.clear(); }
    ~HMGFileModelDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileModelDescription*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBodySubcircuit(ReadALine&, char*, LineInfo&, bool);
    void ReadOrReplaceBodyController(ReadALine&, char*, LineInfo&, bool) {}
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        if (modelType == hfmtSubcircuit) iStream.add(new IsDefModelSubcircuitInstruction(isReplacer, modelIndex, externalNs, internalNs, solutionType, solutionDescriptionIndex));
        else                             iStream.add(new IsDefModelControllerInstruction(isReplacer, modelIndex));
        for (const auto& src: defaults) {
            ForcedNodeDef forcedNodeRange;
            forcedNodeRange.defaultRailIndex = src.rail;
            SimpleNodeID startS, stopS;
            stopS.type = startS.type = src.type;
            startS.index = src.start_index;
            stopS.index  = src.stop_index;
            CDNode startC = SimpleNodeID2CDNode(startS, externalNs, internalNs);
            CDNode stopC  = SimpleNodeID2CDNode(stopS, externalNs, internalNs);
            forcedNodeRange.isExternal = startC.type == cdntExternal;
            forcedNodeRange.nodeStartIndex = startC.index;
            forcedNodeRange.nodeStopIndex  = stopC.index;
            /*
            uns delta = 0;
            switch (src.type) {
                case nvtIO:
                    forcedNodeRange.isExternal = true;
                    break;
                case nvtIN:
                    forcedNodeRange.isExternal = true;
                    delta = externalNs.nIONodes;
                    break;
                case nvtCIN:
                    forcedNodeRange.isExternal = true;
                    delta = externalNs.nIONodes + externalNs.nNormalINodes;
                    break;
                case nvtOUT:
                    forcedNodeRange.isExternal = true;
                    delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes;
                    break;
                case nvtFWOUT:
                    forcedNodeRange.isExternal = true;
                    delta = externalNs.nIONodes + externalNs.nNormalINodes + externalNs.nControlINodes + externalNs.nNormalONodes;
                    break;
                case nvtNInternal:
                    forcedNodeRange.isExternal = false;
                    break;
                case nvtCInternal:
                    forcedNodeRange.isExternal = false;
                    delta = internalNs.nNormalInternalNodes;
                    break;
                default:
                throw hmgExcept("HMGFileModelDescription::toInstructionStream", "DEFAULTRAIL/DEFAULTRAILRANGE: not a node type (%u) connecting to R%u, range: %u-%u", src.type, src.rail, src.start_index, src.stop_index);
            }
            forcedNodeRange.nodeStartIndex = src.start_index + delta;
            forcedNodeRange.nodeStopIndex = src.stop_index + delta;
            */
            iStream.add(new IsRailNodeRangeInstruction(forcedNodeRange));
        }
        for (size_t i = 0; i < instanceList.size(); i++) {
            instanceList[i]->toInstructionStream(iStream);
        }
        if (modelType == hfmtSubcircuit) iStream.add(new IsEndDefInstruction(sitDefModelSubcircuit, modelIndex));
        else                             iStream.add(new IsEndDefInstruction(sitDefModelController, modelIndex));
    }
    //***********************************************************************
    bool readNodeOrParNumber(const char* line, LineTokenizer& lineToken, ReadALine& reader, LineInfo& lineInfo, const char* typeLiteral, uns& destVar) const{
    //***********************************************************************
        if (strcmp(lineToken.getActToken(), typeLiteral) == 0) {
            if (lineToken.isSepEOL)
                throw hmgExcept("HMGFileModelDescription::Read", "%s=number expected, end of line arrived (%s) in %s, line %u", typeLiteral, line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
                throw hmgExcept("HMGFileModelDescription::Read", "%s=number expected, %s arrived (%s) in %s, line %u", typeLiteral, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (sscanf_s(lineToken.getActToken(), "%u", &destVar) != 1)
                throw hmgExcept("HMGFileModelDescription::Read", "%s=number is not a number, %s arrived (%s) in %s, line %u", typeLiteral, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            return true;
        }
        return false;
    }
    //***********************************************************************
    bool checkNodeValidity(SimpleNodeID id) const noexcept{
    //***********************************************************************
        switch(id.type){
            case nvtIO:             return id.index < externalNs.nIONodes;
            case nvtIN:             return id.index < externalNs.nNormalINodes;
            case nvtCIN:            return id.index < externalNs.nControlINodes;
            case nvtOUT:            return id.index < externalNs.nNormalONodes;
            case nvtFWOUT:          return id.index < externalNs.nForwardedONodes;
            case nvtNInternal:      return id.index < internalNs.nNormalInternalNodes;
            case nvtCInternal:      return id.index < internalNs.nControlInternalNodes;
            case nvtVarInternal:    return id.index < internalNs.nInternalVars;
            case nvtParam:          return id.index < externalNs.nParams;
        }
        return true;
    }
};


//***********************************************************************
struct HMGFileSunredTree: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    struct CellID {
    //***********************************************************************
        uns level = 0;
        uns index = 0;
        bool operator<(const CellID& theOther)const noexcept { return level == theOther.level ? (index < theOther.index) : (level < theOther.level); }
    };
    //***********************************************************************
    struct Reduction {
    //***********************************************************************
        ReductionInstruction src;
        bool isValid = false;
    };
    //***********************************************************************
    std::string fullName;
    bool isReplacer = false;
    uns sunredTreeIndex = 0;
    HMGFileSunredTree* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::vector<std::vector<Reduction>> reductions;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileSunredTree*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&, bool);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsDefSunredInstruction(isReplacer, sunredTreeIndex, (uns)reductions.size()));
        for (size_t i = 0; i < reductions.size(); i++) {
            iStream.add(new IsDefSunredLevelInstruction((uns)(i + 1), (uns)reductions[i].size()));
            for(const auto& red : reductions[i])
                iStream.add(new IsDefSunredReductionInstruction(red.src));
        }
        iStream.add(new IsEndDefInstruction(sitSunredTree, sunredTreeIndex));
    }
};


//***********************************************************************
struct HMGFileRails: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    std::vector<std::pair<uns, rvt>> railValues;
    uns nRails = 0; // if 0, no change
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsDefRailsInstruction(nRails, (uns)railValues.size() + 1));
        for (const auto& val : railValues)
            iStream.add(new IsDefRailValueInstruction(val.first, val.second));
        iStream.add(new IsEndDefInstruction(sitRails, 0));
    }
};


//***********************************************************************
struct HMGFileCreate: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    uns fullCircuitIndex = 0;
    uns modelID = 0;
    uns GND = 0; // Rail ID
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsCreateInstruction(fullCircuitIndex, modelID, GND));
    }
};


//***********************************************************************
struct HMGFileProbe: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    uns probeIndex = 0;
    uns probeType = ptV;
    uns fullCircuitID = 0;
    std::vector<ProbeNodeID> nodes;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&, bool);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsProbeInstruction(probeIndex, probeType, fullCircuitID, (uns)nodes.size()));
        for (const auto& node : nodes)
            iStream.add(new IsProbeNodeInstruction(node));
        iStream.add(new IsEndDefInstruction(sitProbe, 0));
    }
};


//***********************************************************************
struct HMGFileRun: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    RunData data;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsRunInstruction(data));
    }
};


//***********************************************************************
struct HMGFileSave: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    bool isRaw = false;
    bool isAppend = false;
    std::string fileName;
    std::vector<uns> probeIDs;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsSaveInstruction(isRaw, isAppend, fileName, (uns)probeIDs.size()));
        for (const auto& probe : probeIDs)
            iStream.add(new IsUnsInstruction(probe));
        iStream.add(new IsEndDefInstruction(sitSave, 0));
    }
};


//***********************************************************************
struct HMGFileGlobalDescription: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    std::list< HMGFileListItem* > itemList;
    //***********************************************************************

    //***********************************************************************
    void clear() { for (auto it : itemList) delete it; itemList.clear(); }
    ~HMGFileGlobalDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        for (HMGFileListItem* it : itemList)
            it->toInstructionStream(iStream);
        iStream.add(new IsEndDefInstruction(sitNothing, 0));
    };
    //***********************************************************************
};




//***********************************************************************
class HmgFileReader {
//***********************************************************************
public:
    inline static HMGFileGlobalDescription globalCircuit;
    void ReadFile(const std::string& fileNameWithPath);
    void convertToInstructionStream(InstructionStream& iStream) {
        iStream.clear();
        globalCircuit.toInstructionStream(iStream);
    }
    void clear() { globalCircuit.clear(); }
};


}


#endif
