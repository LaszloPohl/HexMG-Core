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
#include "hmgCommon.h"
#include "hmgInstructionStream.h"
#include "hmgMultigridTypes.h"
#include "hmgFunction.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


#define MAX_LINE_LENGHT 4096


//***********************************************************************
class LineTokenizer {
//***********************************************************************
    //***********************************************************************
    const char* pLine = nullptr;
    unsigned position = 0, storedPos = 0;
    char token[MAX_LINE_LENGHT] = { 0 };
    bool isSpecialExpressionToken = false;
    enum SpecExprTokenType{ setttNone, settVFirst, settVComma, settVSecond, settVClose };
    SpecExprTokenType specExprToken = setttNone;
    inline static const char* const specCharsOfNames = "_.:@$+-";
    //***********************************************************************
    static bool isSpecChar(char ch, bool isExcept_ = false) {
    //***********************************************************************
        for (const char* pSpec = specCharsOfNames + (isExcept_ ? 1 : 0); *pSpec != 0; pSpec++)
            if (ch == *pSpec)
                return true;
        return false;
    }
    //***********************************************************************
    void setIsSeparators() {
    //***********************************************************************
        isSepSpace = isSepOpeningBracket = isSepClosingBracket = isSepEOL = isSepComma = isSepAssignment = false; 
        unsigned pos;
        bool canBeEOL = true;
        for (pos = position; pLine[pos] == ' ' || pLine[pos] == '(' || pLine[pos] == ')' || pLine[pos] == ',' || pLine[pos] == '='; pos++) {
            if (pLine[pos] == ' ') isSepSpace = true;
            if (pLine[pos] == '(') { isSepOpeningBracket = true; canBeEOL = false; }
            if (pLine[pos] == ')') { isSepClosingBracket = true; canBeEOL = false; }
            if (pLine[pos] == ',') { isSepComma = true; canBeEOL = false; }
            if (pLine[pos] == '=') { isSepAssignment = true; canBeEOL = false; }
        }
        if(canBeEOL && pLine[pos] == 0) isSepEOL = true;
    }
    //***********************************************************************
public:
    //***********************************************************************
    bool isSepSpace = false, isSepOpeningBracket = false, isSepClosingBracket = false, isSepEOL = false, isSepComma = false, isSepAssignment = false;
    std::string errorMessage;
    //***********************************************************************
    void init(const char* theLine) { pLine = theLine; position = 0; setIsSeparators(); }
    //***********************************************************************
    void storePosition() { storedPos = position; }
    void loadPosition() { position = storedPos; }
    //***********************************************************************
    void skipSeparators() {
    //***********************************************************************
        while (pLine[position] == ' ' || pLine[position] == '(' || pLine[position] == ')' || pLine[position] == ',' || pLine[position] == '=')
            position++;
    }
    //***********************************************************************
    void skipSeparatorsUntilClosingBracket() {
    //***********************************************************************
        while (pLine[position] == ' ' || pLine[position] == '(' || pLine[position] == ')' || pLine[position] == ',' || pLine[position] == '=') {
            if (pLine[position] == ')') {
                position++;
                break;
            }
            position++;
        }
    }
    //***********************************************************************
    void skipSeparatorsAndSetIsSeparators() {
    //***********************************************************************
        skipSeparators();
        setIsSeparators();
    }
    //***********************************************************************
    bool getIsControl() {
    //***********************************************************************
        skipSeparators();
        if (pLine[position] == '>') {
            position++;
            return true;
        }
        return false;
    }
    //***********************************************************************
    const char* getLine()const { return pLine; }
    //***********************************************************************
    char* getNextToken(const char* fileName, unsigned lineNumber) {
    // from position the alphanum chars + specCharsOfNames, skips starting
    // separators
    //***********************************************************************
        skipSeparators();
        unsigned tokenpos = 0;
        while (isalnum(pLine[position]) || isSpecChar(pLine[position]))
            token[tokenpos++] = pLine[position++];
        token[tokenpos] = 0;
        if (tokenpos == 0)
            throw hmgExcept("LineTokenizer::getNextToken", "unexpected end of line in %s, line %u: %s", fileName, lineNumber, pLine);
        setIsSeparators();
        return token;
    }
    //***********************************************************************
    bool getNextTokenSimple(const char* fileName, unsigned lineNumber) {
    //***********************************************************************
        getNextToken(fileName, lineNumber);
        return !isSpecChar(token[0], true);
    }
    //***********************************************************************
    const char* getActToken()const {
    //***********************************************************************
        return token;
    }
    //***********************************************************************
    const char* getRestOfTheLine() {
    // the expression from the end of the line
    //***********************************************************************
        while (pLine[position] == ' ')
            position++;
        unsigned tokenpos = 0;
        while (pLine[position] != 0)
            token[tokenpos++] = pLine[position++];
        token[tokenpos] = 0;
        setIsSeparators();
        return token;
    }
    //***********************************************************************
    bool getQuotedText(const char* & ret) {
    //***********************************************************************
        skipSeparators();
        if (pLine[position] != '\"') {
            token[0] = 0;
            setIsSeparators();
            ret = token;
            return false;
        }
        unsigned tokenpos = 0;
        position++;
        while (pLine[position] != '\"' && pLine[position] != 0) {
            token[tokenpos++] = pLine[position++];
        }
        token[tokenpos] = 0;
        ret = token;
        bool rv = pLine[position] != 0;
        if (rv)
            position++;
        setIsSeparators();
        return rv;
    }
    //***********************************************************************
    bool getQuotedText() {
    //***********************************************************************
        const char* temp;
        return getQuotedText(temp);
    }
};


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
inline bool textToSimpleInterfaceNodeID(const char* text, SimpleInterfaceNodeID& result, std::map<std::string, uns> &globalVarNames, bool isNodeN = true) {
//***********************************************************************
    result.isStepStart = false;
    for (uns i = 0; text[i] != '\0'; i++)
        if (text[i] == '.') {
            if (strcmp(text + i + 1, "STEPSTART") == 0)
                result.isStepStart = true;
            break;
        }
    switch (text[0]) {
        case 'A':
            result.type = nvtA;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        case 'B':
            if (text[1] == 'G') {
                result.type = nvtBG;
                if (isNodeN) {
                    if (!globalVarNames.contains(text)) {
                        result.index = (uns)globalVarNames.size();
                        globalVarNames[text] = result.index;
                    }
                    else
                        result.index = globalVarNames[text];
                }
                else 
                    return false; // There is no index for global variables, it contains a label.
            }
            else {
                result.type = nvtB;
                if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                    return false;
            }
            break;
        case 'G':
            if (text[1] == 'N' && text[2] == 'D') {
                result.type = nvtGND;
            }
            else
                return false;
            break;
        case 'N':
            if (text[1] == 'O' && text[2] == 'N' && text[3] == 'E') {
                result.type = nvtUnconnected; // ! nvtNone is used for other purposes
            }
            else {
                result.type = nvtN;
                if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                    return false;
            }
            break;
        case 'O':
            result.type = nvtO;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        case 'P':
            result.type = nvtParam;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        case 'R':
            result.type = nvtRail;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        case 'X':
            result.type = nvtX;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        case 'Y':
            result.type = nvtY;
            if (isNodeN && sscanf_s(&text[1], "%u", &result.index) != 1)
                return false;
            break;
        default:
            return false;
    }
    return true;
}


//***********************************************************************
inline bool textToCDNode(const char* text, CDNode& result, std::map<std::string, uns>& globalVarNames) {
//***********************************************************************
    SimpleInterfaceNodeID tmp;
    if (!textToSimpleInterfaceNodeID(text, tmp, globalVarNames))
        return false;
    if (tmp.type != nvtX && tmp.type != nvtN)
        return false;
    result.index = tmp.index;
    result.type = tmp.type == nvtX ? cdntExternal : cdntInternal;
    return true;
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
struct HMGFileFunction;
//***********************************************************************


//***********************************************************************
struct GlobalHMGFileNames {
//***********************************************************************
    std::map<std::string, uns> modelNames, sunredTreeNames, multigridNames,
        localRestrictionTypeNames, localProlongationTypeNames,
        globalVarNames, functionNames, probeNames, fullCircuitNames;
    std::vector<HMGFileModelDescription*> modelData;
    std::vector<HMGFileProbe*> probeData;
    std::vector<HMGFileCreate*> fullCircuitData;
    std::vector<HMGFileFunction*> functionData;
    //***********************************************************************
    bool textToDeepInterfaceNodeID(char* token, uns fullCircuitIndex, DeepInterfaceNodeID& dest);
    bool textToDeepInterfaceVarID(char* token, DeepInterfaceNodeID& dest);
    bool textToDeepCDNodeID(char* token, uns fullCircuitIndex, DeepCDNodeID& dest);
    bool textRawToDeepCDNodeID(char* token, DeepCDNodeID& dest);
    //***********************************************************************
};


//***********************************************************************
struct HMGFileListItem {
//***********************************************************************
    inline static GlobalHMGFileNames globalNames;
    inline static uns globalNRails = 0; // for checking
    virtual ~HMGFileListItem() {}
    virtual void toInstructionStream(InstructionStream& iStream) = 0;
    //***********************************************************************
    bool readNodeOrParNumber(const char* line, LineTokenizer& lineToken, ReadALine& reader, LineInfo& lineInfo, const char* typeLiteral, uns& destVar) const{
    //***********************************************************************
        if (strcmp(lineToken.getActToken(), typeLiteral) == 0) {
            if (lineToken.isSepEOL)
                throw hmgExcept("HMGFileListItem::Read", "%s=number expected, end of line arrived (%s) in %s, line %u", typeLiteral, line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
                throw hmgExcept("HMGFileListItem::Read", "%s=number expected, %s arrived (%s) in %s, line %u", typeLiteral, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (sscanf_s(lineToken.getActToken(), "%u", &destVar) != 1)
                throw hmgExcept("HMGFileListItem::Read", "%s=number is not a number, %s arrived (%s) in %s, line %u", typeLiteral, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            return true;
        }
        return false;
    }
};


//***********************************************************************
struct HMGFileComponentInstanceLine : HMGFileListItem {
//***********************************************************************
    uns instanceIndex = 0; // component index or controller index

    uns modelIndex = 0; // modelNames, BuiltInComponentTemplateType
    std::vector<ComponentIndex> componentParams;   // unsMax means _THIS
    std::vector<SimpleInterfaceNodeID> nodes;
    std::vector<ParameterInstance> params;
    bool isDefaultRail = false;
    bool isController = false;
    bool isBuiltIn = false;
    uns defaultValueRailIndex = 0;
    uns ctrlLevel = 0;
    //***********************************************************************
    // function controlled component:
    bool isFunctionControlled = false;
    uns nIN = 0;
    uns nCin = 0;
    uns nPar = 0;
    uns nCT = 0;                    // nCT and componentParams.size() must be the same at the end of reading => no need to send both to the instruction stream
    bool isFunctionBuiltIn = false;
    uns functionIndex = 0;
    std::vector<uns> functionComponentParams;   // unsMax means _THIS
    std::vector<SimpleInterfaceNodeID> functionParams;
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        if(isFunctionControlled){
            iStream.add(new IsFunctionControlledComponentInstanceInstruction(instanceIndex, modelIndex, isDefaultRail, 
                defaultValueRailIndex, isController, isBuiltIn, (uns)nodes.size(), (uns)params.size(), (uns)componentParams.size(), nIN, nCin, nPar,
                isFunctionBuiltIn, functionIndex, (uns)functionParams.size(), (uns)functionComponentParams.size()));
            for (const auto& fparam : functionParams) // not sending param for RET (controller does)
                iStream.add(new IsNodeValueInstruction(fparam));
            for (const auto& num : functionComponentParams)
                iStream.add(new IsUnsInstruction(num));
            for (const auto& node : nodes)
                iStream.add(new IsNodeValueInstruction(node));
            for (const auto& param : params)
                iStream.add(new IsParameterValueInstruction(param));
            for (const auto& cp : componentParams)
                iStream.add(new IsComponentIndexInstruction(cp));
            iStream.add(new IsEndDefInstruction(sitComponentInstance, instanceIndex)); // ! sitComponentInstance (ComponentDefinition::processInstructions() expects sitComponentInstance)
        }
        else {
            iStream.add(new IsComponentInstanceInstruction(instanceIndex, modelIndex, isDefaultRail, defaultValueRailIndex, isController, ctrlLevel, isBuiltIn, (uns)nodes.size(), (uns)params.size(), (uns)componentParams.size()));
            for (const auto& node : nodes)
                iStream.add(new IsNodeValueInstruction(node));
            for (const auto& param : params)
                iStream.add(new IsParameterValueInstruction(param));
            for (const auto& cp : componentParams)
                iStream.add(new IsComponentIndexInstruction(cp));
            iStream.add(new IsEndDefInstruction(sitComponentInstance, instanceIndex));
        }
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
    InternalNodeSizePack internalNs;
    HMGFileModelDescription* pParent = nullptr; // if this is a replacer, parent is the replaced object
    //***********************************************************************
    //                        ***** subcircuit data *****
    //***********************************************************************
    std::vector< HMGFileComponentInstanceLine* > instanceList;
    std::map<std::string, uns> componentInstanceNameIndex;
    std::map<std::string, uns> controllerInstanceNameIndex;
    std::map<std::string, uns> instanceListIndex;
    std::vector<DefaultRailRange> defaults;
    SolutionType solutionType = stFullMatrix;
    uns solutionDescriptionIndex = 0; // for sunred and multigrid 
    //***********************************************************************
    //                        ***** controller data *****
    //***********************************************************************
    std::vector<DefaultNodeParameter> defaultNodeValues;
    builtInFunctionType functionType = biftInvalid;
    uns functionCustomIndex = unsMax; // if functionType == biftCustom => index in globalNames.functionNames
    std::vector<uns> functionComponentParams;   // unsMax means _THIS
    std::vector<SimpleInterfaceNodeID> functionParamsLoad;
    std::vector<SimpleInterfaceNodeID> functionParamsStore;
    //***********************************************************************

    //***********************************************************************
    void clear() { for (auto it : instanceList) delete it; instanceList.clear(); }
    ~HMGFileModelDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileModelDescription*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBodySubcircuit(ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBodyController(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        if (modelType == hfmtSubcircuit) {
            iStream.add(new IsDefModelSubcircuitInstruction(isReplacer, modelIndex, externalNs, internalNs, solutionType, solutionDescriptionIndex));
            for (const auto& src : defaults) {
                ForcedNodeDef forcedNodeRange;
                forcedNodeRange.defaultRailIndex = src.rail;
                SimpleInterfaceNodeID startS, stopS;
                stopS.type = startS.type = src.type;
                startS.index = src.start_index;
                stopS.index = src.stop_index;
                CDNode startC = SimpleInterfaceNodeID2CDNode(startS, externalNs, internalNs);
                CDNode stopC = SimpleInterfaceNodeID2CDNode(stopS, externalNs, internalNs);
                forcedNodeRange.isExternal = startC.type == cdntExternal;
                forcedNodeRange.nodeStartIndex = startC.index;
                forcedNodeRange.nodeStopIndex = stopC.index;
                iStream.add(new IsRailNodeRangeInstruction(forcedNodeRange));
            }
            for (size_t i = 0; i < instanceList.size(); i++) {
                instanceList[i]->toInstructionStream(iStream);
            }
            iStream.add(new IsEndDefInstruction(sitDefModelSubcircuit, modelIndex));
        }
        else {
            iStream.add(new IsDefModelControllerInstruction(isReplacer, modelIndex, externalNs, internalNs, functionType, functionCustomIndex,
                (uns)defaultNodeValues.size(), (uns)functionComponentParams.size(), (uns)functionParamsLoad.size(), (uns)functionParamsStore.size()));
            for (const auto& dnv : defaultNodeValues)
                iStream.add(new IsDefaultNodeParameterInstruction(dnv));
            for (const auto& fcp : functionComponentParams)
                iStream.add(new IsUnsInstruction(fcp));
            for (const auto& fpl : functionParamsLoad) // sending param for RET (funct controlled components does not)
                iStream.add(new IsNodeValueInstruction(fpl));
            for (const auto& fps : functionParamsStore) // sending param for RET (funct controlled components does not)
                iStream.add(new IsNodeValueInstruction(fps));
            iStream.add(new IsEndDefInstruction(sitDefModelController, modelIndex));
        }
    }
    //***********************************************************************
    bool checkNodeValidity(SimpleInterfaceNodeID id) const noexcept{
    //***********************************************************************
        switch(id.type){
            case nvtX:      return id.index < externalNs.nXNodes;
            case nvtY:      return id.index < externalNs.nYNodes;
            case nvtA:      return id.index < externalNs.nANodes;
            case nvtO:      return id.index < externalNs.nONodes;
            case nvtN:      return id.index < internalNs.nNNodes;
            case nvtB:      return id.index < internalNs.nBNodes;
            case nvtParam:  return id.index < externalNs.nParams;
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
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&);
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
struct HMGFileMultiGrid : HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    uns multigridIndex = 0;
    bool isReplacer = false;
    std::string fullName;
    HMGFileMultiGrid* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::vector<LocalProlongationOrRestrictionInstructions> localNodeRestrictionTypes;
    std::vector<LocalProlongationOrRestrictionInstructions> localNodeProlongationTypes;
    std::vector<InterfaceFineCoarseConnectionDescription> levels; // 0 is the coarsest multigrid level, sunred level is not included because these are the destination levels
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileMultiGrid*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsDefMultigridInstruction(isReplacer, multigridIndex, (uns)localNodeRestrictionTypes.size(), (uns)localNodeProlongationTypes.size(), (uns)levels.size()));
        
        for (uns i = 0; i < localNodeRestrictionTypes.size(); i++)
            localNodeRestrictionTypes[i].toInstructionStream(iStream, true, i);

        for (uns i = 0; i < localNodeProlongationTypes.size(); i++)
            localNodeProlongationTypes[i].toInstructionStream(iStream, false, i);

        for (const auto& level : levels)
            level.toInstructionStream(iStream);

        iStream.add(new IsEndDefInstruction(sitMultigrid, multigridIndex));
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
struct HMGFileFunction: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    struct FunctionDescription {
    //***********************************************************************
        builtInFunctionType type = biftInvalid;
        uns customIndex;                                                        // if type == biftCustom => index in globalNames.functionNames
        std::vector<uns> componentParams;
        std::vector<ParameterIdentifier> parameters;
        std::vector<rvt> values;                                                // function parameter values for _PWL
        rvt value = rvt0;                                                       // function parameter value for _CONST
        uns labelXID = 0;                                                       // for jump instructions, also the index of the external source, e.g. CT6.X2 => labelXID = 6, xSrc = { nvtX, 2 }
        SimpleInterfaceNodeID xSrc;                                             // nodeID of the external source
        std::string labelName;                                                  // first read only the name of the label beacuse forward jumping is possible
    };
    //***********************************************************************
    uns functionIndex = 0;
    uns nParams = 0;
    uns nVars = 0;
    uns nComponentParams = 0;
    std::map<std::string, uns> labels;
    std::map<rvt, uns> constants; // constant, variable index
    std::vector<FunctionDescription> instructions;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void ReadComponentParams(FunctionDescription& dest, uns nPar, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo) {
    //***********************************************************************
        dest.componentParams.reserve(nPar);
        for (uns i = 0; i < nPar; i++) {
            char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            uns ct = 0;
            if (strcmp(token, "_THIS") == 0)
                ct = unsMax;
            else if (token[0] == 'C' && token[1] == 'T') {
                if (sscanf_s(token + 2, "%u", &ct) != 1)
                    throw hmgExcept("HMGFileFunction::ReadParams", "CTnumber expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }
            else
                throw hmgExcept("HMGFileFunction::ReadParams", "CT or _THIS expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

            dest.componentParams.push_back(ct);
        }
    }
    //***********************************************************************
    void ReadParams(FunctionDescription& dest, uns nPar, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo);
    //***********************************************************************
    void ReadLabel(FunctionDescription& dest, LineTokenizer& lineToken, ReadALine& reader, LineInfo& lineInfo) {
    //***********************************************************************
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        dest.labelName = token;
        dest.labelXID = unsMax;
    }
    //***********************************************************************
    void ReadValue(FunctionDescription& dest, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo) {
    //***********************************************************************
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (!spiceTextToRvt(token, dest.value))
            throw hmgExcept("HMGFileFunction::ReadValue", "value is not a number: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }
    /*
    //***********************************************************************
    void ReadVG(FunctionDescription& dest, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo) {
    //***********************************************************************
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (token[0] != 'V' || token[1] != 'G')
            throw hmgExcept("HMGFileFunction::ReadVG", "global variable name (VG) expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        if (sscanf_s(token + 2, "%u", &dest.labelXID) != 1)
            throw hmgExcept("HMGFileFunction::ReadVG", "global variable name (VG) expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }
    */
    //***********************************************************************
    void ReadNodeVariable(FunctionDescription& dest, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo, bool isNodeN) {
    //***********************************************************************
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (token[0] == 'B' && token[1] == 'G') {
            dest.xSrc.type = nvtBG;
            if (isNodeN) {
                if (!globalNames.globalVarNames.contains(token)) {
                    dest.xSrc.index = (uns)globalNames.globalVarNames.size();
                    globalNames.globalVarNames[token] = dest.xSrc.index;
                }
                else
                    dest.xSrc.index = globalNames.globalVarNames[token];
            }
            dest.componentParams.push_back(unsMax);
        }
        else if (token[0] == 'C' && token[1] == 'T') {
            char* token2 = token + 2;
            if (sscanf_s(token2, "%u", &dest.labelXID) != 1)
                throw hmgExcept("HMGFileFunction::ReadNodeVariable", "CT name with an index expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            while (*token2 != '.' && *token2 != 0)
                token2++;
            if (*token2 == 0)
                throw hmgExcept("HMGFileFunction::ReadNodeVariable", "missing node name after CT: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            token2++;
            if (!textToSimpleInterfaceNodeID(token2, dest.xSrc, globalNames.globalVarNames, isNodeN))
                throw hmgExcept("HMGFileFunction::ReadNodeVariable", "unrecognized node ID: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            dest.componentParams.push_back(dest.labelXID);
        }
        else
            throw hmgExcept("HMGFileFunction::ReadNodeVariable", "unrecognized node name: %s (CT or VG expected) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsFunctionInstruction(functionIndex, nComponentParams, nParams, nVars, (uns)instructions.size()));
        for (const auto& line : instructions) {
            iStream.add(new IsFunctionCallInstruction(line.type, line.customIndex, line.value, line.labelXID, line.xSrc, (uns)line.componentParams.size(), (uns)line.parameters.size(), (uns)line.values.size()));
            for (const auto& par : line.componentParams)
                iStream.add(new IsUnsInstruction(par));
            for (const auto& par : line.parameters)
                iStream.add(new IsFunctionParIDInstruction(par));
            for (const auto& val : line.values)
                iStream.add(new IsRvtInstruction(val));
        }
        iStream.add(new IsEndDefInstruction(sitFunction, 0));
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
    uns xy_k = 0; // FIM probe
    uns z_k  = 0; // FIM probe
    std::vector<DeepInterfaceNodeID> nodes;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&, bool);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsProbeInstruction(probeIndex, probeType, fullCircuitID, (uns)nodes.size(), xy_k, z_k));
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
struct HMGFileSet: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    DeepInterfaceNodeID varID; // contains the FullCircuitID !
    rvt value = rvt0;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsSetInstruction(varID, value));
    }
};


//***********************************************************************
struct HMGFileSave: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    bool isRaw = false;
    bool isAppend = false;
    bool isFIM = false;
    uns maxResultsPerRow = 100;
    std::string fileName;
    std::vector<uns> probeIDs;
    //***********************************************************************

    //***********************************************************************
    void Read(ReadALine&, char*, LineInfo&);
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override {
    //***********************************************************************
        iStream.add(new IsSaveInstruction(isFIM, isRaw, isAppend, maxResultsPerRow, fileName, (uns)probeIDs.size()));
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
