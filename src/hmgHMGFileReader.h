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
#include "NameToIndex.h"
#include "hmgCommon.h"
#include "InstructionStream.h"
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
struct SpiceSubcktDescription;
struct SpiceControllerDescription;
//***********************************************************************


//***********************************************************************
struct GlobalHMGFileNames {
//***********************************************************************
    std::map<std::string, uns> modelNames, sunredTreeNames, varNames, functionNames, probeNames, fullCircuitNames;
    std::vector<HMGFileSunredTree*> sunredTreeData;
    std::vector<HMGFileModelDescription*> modelData;
};


//***********************************************************************
struct HMGFileListItem {
//***********************************************************************
    inline static GlobalHMGFileNames globalNames;
    inline static uns globalNRails = 0; // for checking
    virtual ~HMGFileListItem() {}
    virtual HMGFileInstructionType getItemType()const = 0;
    virtual void toInstructionStream(InstructionStream& iStream) = 0;
};


//***********************************************************************
struct HMGFileComponentInstanceLine : HMGFileListItem {
//***********************************************************************
    uns instanceIndex = 0; // component index or controller index

    HMGFileInstructionType instanceOfWhat = itNone; // itModel, itBuiltInComponentType
    uns indexOfTypeInGlobalContainer = 0; // modelNames, BuiltInComponentTemplateType
    std::vector<SimpleNodeID> nodes;
    std::vector<ParameterInstance> params;
    bool isDefaultRail = false;
    uns defaultRailIndex = 0;
    //SpiceExpression valueExpression; // for built in components
    //unsigned short nNormalNode, nControlNode, nParams;

    //HMGFileComponentInstanceLine() :nNormalNode{ 0 }, nControlNode{ 0 }, nParams{ 0 }{}

    HMGFileInstructionType getItemType()const override { return itComponentInstance; }
    void toInstructionStream(InstructionStream& iStream)override;
};


//***********************************************************************
struct HMGFileModelDescription: HMGFileListItem {
//***********************************************************************
    enum ModelType{ hfmtSubcircuit, hfmtController };
    std::string fullName;
    uns modelIndex = 0;
    ModelType modelType = hfmtSubcircuit;
    bool isReplacer = false;
    //***********************************************************************
    // External nodes:
    uns nIONodes = 0;
    uns nNormalINodes = 0;
    uns nControlNodes = 0;
    uns nNormalONodes = 0;
    uns nForwardedONodes = 0;
    uns sumExternalNodes = 0;
    // Internal nodes:
    uns nNormalInternalNodes = 0;
    uns nControlInternalNodes = 0;
    uns nInternalVars = 0;
    uns sumInternalNodes = 0;
    //***********************************************************************
    uns nParams = 0;
    HMGFileModelDescription* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::list< HMGFileListItem* > itemList;
    std::map<std::string, uns> componentInstanceNameIndex;
    std::map<std::string, uns> controllerInstanceNameIndex;
    std::vector<std::tuple<uns, NodeVarType, uns, uns>> defaults; // <rail, type, start_index, stop_index>

    //***********************************************************************
    void clear() { for (auto it : itemList) delete it; itemList.clear(); }
    ~HMGFileModelDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileModelDescription*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBodySubcircuit(ReadALine&, char*, LineInfo&, bool);
    void ReadOrReplaceBodyController(ReadALine&, char*, LineInfo&, bool) {}
    void ProcessXLines(); // a subcontrollerre is figyelni! ha replace, akkor figyeljen a node-ok indexére! the definition of .subckt/.component/.controller can be later than its instantiation so we don't now the type of the nodes when an X line arrives
    void ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList);
    HMGFileInstructionType getItemType()const override { return itModel; }
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override;
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
            case nvtIO:             return id.index < nIONodes;
            case nvtIN:             return id.index < nNormalINodes;
            case nvtCIN:            return id.index < nControlNodes;
            case nvtOUT:            return id.index < nNormalONodes;
            case nvtFWOUT:          return id.index < nForwardedONodes;
            case nvtNInternal:      return id.index < nNormalInternalNodes;
            case nvtCInternal:      return id.index < nControlInternalNodes;
            case nvtVarInternal:    return id.index < nInternalVars;
            case nvtParam:          return id.index < nParams;
        }
        return true;
    }
};


//***********************************************************************
struct SpiceExpressionLine: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    uns expressionIndex;
    SpiceExpression theExpression;

    SpiceExpressionLine() :expressionIndex{ 0 } {}
    void Read(ReadALine&, char*, LineInfo&);
    HMGFileInstructionType getItemType()const override { return itExpression; }
    void toInstructionStream(InstructionStream& iStream)override { theExpression.toInstructionStream(iStream, expressionIndex); }
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
        CellID src1;
        CellID src2;
        CellID dest;
    };
    //***********************************************************************
    std::string fullName;
    bool isReplacer;
    uns sunredTreeIndex = 0;
    HMGFileSunredTree* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::vector<Reduction> reductions;

    void Read(ReadALine&, char*, LineInfo&);
    void Replace(HMGFileSunredTree*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&, bool);
    HMGFileInstructionType getItemType()const override { return itSunredTree; }
    void toInstructionStream(InstructionStream& iStream)override {}; // TODO !!!
};


//***********************************************************************
struct HMGFileRails: HMGFileListItem {
//***********************************************************************
    std::vector<std::pair<uns, rvt>> railValues;
    uns nRails = 0; // if 0, no change

    void Read(ReadALine&, char*, LineInfo&);
    HMGFileInstructionType getItemType()const override { return itRail; }
    void toInstructionStream(InstructionStream& iStream)override {}; // TODO !!!
};


//***********************************************************************
struct HMGFileGlobalDescription: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    uns subcktIndex;
    std::list< HMGFileListItem* > itemList;
    std::map<std::string, uns> modelNameIndex;

    //***********************************************************************
    HMGFileGlobalDescription() :subcktIndex{ 0 }{}
    void clear() { for (auto it : itemList) delete it; itemList.clear(); }
    ~HMGFileGlobalDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&);
    void ProcessXLines() {}; // TODO !!! // a subcontrollerre is figyelni! ha replace, akkor figyeljen a node-ok indexére! the definition of .subckt/.component/.controller can be later than its instantiation so we don't now the type of the nodes when an X line arrives
    void ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList) {}; // TODO !!!
    HMGFileInstructionType getItemType()const override { return itGlobal; }
    void toInstructionStream(InstructionStream& iStream)override {}; // TODO !!!
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
