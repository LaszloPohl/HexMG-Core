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
        return ret;
    }
    //***********************************************************************
    int peekChar()const {
    //***********************************************************************
        return (buffer1->index == buffer1->elementNum) ? EOF : buffer1->buffer[buffer1->index];
    }
    //***********************************************************************
    int peekChar2()const { // peeks the 2nd char
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
struct SpiceSubcktDescription;
struct SpiceControllerDescription;
//***********************************************************************


//***********************************************************************
struct GlobalHMGFileNames {
//***********************************************************************
    std::map<std::string, uns> modelNames, sunredTreeNames, varNames, functionNames, probeNames, fullCircuitNames;
    std::vector<HMGFileSunredTree*> sunredTreeData;
};


//***********************************************************************
struct HMGFileListItem {
//***********************************************************************
    inline static GlobalHMGFileNames globalNames;
    virtual ~HMGFileListItem() {}
    virtual HMGFileInstructionType getItemType()const = 0;
    virtual void toInstructionStream(InstructionStream& iStream) = 0;
};


//***********************************************************************
struct SpiceComponentInstanceLine : HMGFileListItem {
//***********************************************************************
    std::string theLine, fileName;
    LineInfo theLineInfo;

    unsigned componentInstanceIndex;

    HMGFileInstructionType instanceOfWhat; // subckt, controller, componentTemplate, builtInComponent
    unsigned indexOfTypeInGlobalContainer; // subcktNames, controllerNames, componentTemplateNames, BuiltInComponentTemplateType
    std::vector<NodeID> nodes;
    std::vector<ParameterInstance> params;
    SpiceExpression valueExpression; // for built in components
    unsigned short nNormalNode, nControlNode, nParams;

    SpiceComponentInstanceLine() :componentInstanceIndex{ 0 }, instanceOfWhat{ itNone }, indexOfTypeInGlobalContainer{ 0 }, nNormalNode{ 0 }, nControlNode{ 0 }, nParams{ 0 }{}

    HMGFileInstructionType getItemType()const override { return itComponentInstance; }
    void toInstructionStream(InstructionStream& iStream)override;
};


//***********************************************************************
struct SpiceSubcktDescription: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    unsigned subcktIndex;
    bool isReplacer;
    unsigned short nNormalNode, nControlNode; // number of noprmal and control nodes
    unsigned short nParams; // number of parameters
    SpiceSubcktDescription* pParent; // if this is a replacer, parent is the replaced object
    std::list< HMGFileListItem* > itemList;
    std::map<std::string, uns> componentInstanceNameIndex;

    //***********************************************************************
    SpiceSubcktDescription() :subcktIndex{ 0 }, isReplacer{ false }, nNormalNode{ 0 }, nControlNode{ 0 },
        nParams{ 0 }, pParent{ nullptr } {}
    void clear() { for (auto it : itemList) delete it; itemList.clear(); }
    ~SpiceSubcktDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&, const std::string&);
    void Replace(SpiceSubcktDescription*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&, bool);
    void ProcessXLines(); // a subcontrollerre is figyelni! ha replace, akkor figyeljen a node-ok indexére! the definition of .subckt/.component/.controller can be later than its instantiation so we don't now the type of the nodes when an X line arrives
    void ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList);
    HMGFileInstructionType getItemType()const override { return itSubckt; }
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override;
    //***********************************************************************
};


//***********************************************************************
struct SpiceControllerDescription: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    bool isReplacer;
    unsigned controllerIndex;
    unsigned short nControlNode; // number of control nodes
    unsigned short nParams; // number of parameters
    SpiceControllerDescription* pParent; // if this is a replacer, parent is the replaced object
    std::list< HMGFileListItem* > itemList;

    //***********************************************************************
    SpiceControllerDescription() :controllerIndex{ 0 }, nControlNode{ 0 }, nParams{ 0 }, isReplacer{ false }, pParent{ nullptr } {}
    ~SpiceControllerDescription() { for (auto it : itemList) delete it; }
    void Read(ReadALine&, char*, LineInfo&, const std::string&);
    void Replace(SpiceControllerDescription*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&, bool);
    void ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList);
    HMGFileInstructionType getItemType()const override { return itController; }
    //***********************************************************************
    void toInstructionStream(InstructionStream& iStream)override;
    //***********************************************************************
};


//***********************************************************************
struct SpiceExpressionLine: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    unsigned expressionIndex;
    SpiceExpression theExpression;

    SpiceExpressionLine() :expressionIndex{ 0 } {}
    void Read(ReadALine&, char*, LineInfo&, const std::string&);
    HMGFileInstructionType getItemType()const override { return itExpression; }
    void toInstructionStream(InstructionStream& iStream)override { theExpression.toInstructionStream(iStream, expressionIndex); }
};


//***********************************************************************
struct HMGFileSunredTree: HMGFileListItem {
//***********************************************************************
    //***********************************************************************
    struct Reduction {
    //***********************************************************************
        uns src1Level = 0, src1Index = 0;
        uns src2Level = 0, src2Index = 0;
        uns destLevel = 0, destIndex = 0;
    };
    //***********************************************************************
    std::string fullName;
    bool isReplacer;
    unsigned sunredTreeIndex = 0;
    HMGFileSunredTree* pParent = nullptr; // if this is a replacer, parent is the replaced object
    std::vector<Reduction> reductions;

    void Read(ReadALine&, char*, LineInfo&, const std::string&);
    void Replace(HMGFileSunredTree*, ReadALine&, char*, LineInfo&);
    void ReadOrReplaceBody(ReadALine&, char*, LineInfo&, bool);
    HMGFileInstructionType getItemType()const override { return itExpression; }
    void toInstructionStream(InstructionStream& iStream)override {}; // TODO !!!
};


//***********************************************************************
struct HMGFileGlobalDescription: HMGFileListItem {
//***********************************************************************
    std::string fullName;
    unsigned subcktIndex;
    std::list< HMGFileListItem* > itemList;
    std::map<std::string, uns> modelNameIndex;

    //***********************************************************************
    HMGFileGlobalDescription() :subcktIndex{ 0 }{}
    void clear() { for (auto it : itemList) delete it; itemList.clear(); }
    ~HMGFileGlobalDescription() { clear(); }
    void Read(ReadALine&, char*, LineInfo&, const std::string&);
    void ProcessXLines() {}; // TODO !!! // a subcontrollerre is figyelni! ha replace, akkor figyeljen a node-ok indexére! the definition of .subckt/.component/.controller can be later than its instantiation so we don't now the type of the nodes when an X line arrives
    void ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList) {}; // TODO !!!
    HMGFileInstructionType getItemType()const override { return itSubckt; }
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
