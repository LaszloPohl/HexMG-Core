//***********************************************************************
// HexMG HMG File Converter Source
// Creation date:  2021. 06. 20.
// Creator:        L�szl� Pohl
//***********************************************************************

//***********************************************************************
#define _CRT_SECURE_NO_WARNINGS
//***********************************************************************
#include <iostream>
#include "hmgHMGFileReader.h"
#include <cstdio>
#include <stack>
#include <set>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
std::string getPath(const std::string& fileNameWithPath) {
//***********************************************************************
    size_t separator = fileNameWithPath.length(); // points to the terminating zero
    while (separator > 0 && fileNameWithPath[separator] != '/' && fileNameWithPath[separator] != '\\')
        separator--;
    if (separator == 0)
        return std::string{ "" };
    return fileNameWithPath.substr(0, separator + 1);
}


//***********************************************************************
bool ReadALine::open(const std::string& filePath, const std::string& fileName) {
//***********************************************************************
    fileNameIndex = unsigned(names.size());
    names.push_back(FileNameAndPath(filePath, fileName));
    if (fp != nullptr)
        fclose(fp);
    fp = fopen((filePath + fileName).c_str(), "rb");
    is_open = fp != nullptr;
    is_EOF = !is_open;
    actLine = 0;
    if (is_EOF)
        return false;
    readNextBlock();
    return true;
}


//***********************************************************************
void ReadALine::appendLine(char* result, unsigned resultSize, LineInfo& lineInfo) {
//***********************************************************************
    if (!is_open || fp == nullptr || resultSize == 0)
        return;
    if (pInclude != nullptr) {
        pInclude->appendLine(result, resultSize, lineInfo);
        if (pInclude->isEOF()) {
            delete pInclude;
            pInclude = nullptr;
        }
        return;
    }
    actLine++;
    bool isNotFinished = true, isPeek = true;
    bool inQuote = false;
    enum States { stStart, stEOF, stEOL, stCommentLine };
    States st = stStart;
    while (isNotFinished) {
        int ch = isPeek ? peekChar() : getChar();
        if (ch == EOF)
            st = stEOF;
        else if (ch == '\n')
            st = stEOL;
        if (ch == '\"')
            inQuote = !inQuote;
        if (!inQuote)
            ch = toupper(ch);
        switch (st) {
            case stStart:
                if (isspace(ch) || ch == '*' || ch == ';') {
                    st = stCommentLine;
                    getChar();
                    isPeek = false;
                }
                else if (ch == '/' && (isPeek ? peekChar2() == '/' : peekChar() == '/')) {
                    st = stCommentLine;
                    getChar();
                    getChar();
                    isPeek = false;
                }
                else if (ch == '+') {
                    actLine--;
                    LineInfo liTemp = lineInfo;
                    unsigned eolIndex = 0;
                    while (result[eolIndex] != 0)
                        eolIndex++;
                    getChar(); // ignore '+'
                    getLine(result + eolIndex, resultSize - eolIndex, liTemp, true);
                    lineInfo.lastLine = liTemp.lastLine;
                    return;
                }
                else {
                    actLine--;
                    return;
                }
                break;
            case stCommentLine: // ignores everything in the line
                break;
        }
        if (st == stEOL) {
            if(isPeek)
                getChar();
            if (peekChar() == '\r') // if the line is finished with \n\r
                getChar();
            actLine++;
            st = stStart;
            isPeek = true;
        }
        if (st == stEOF) {
            isNotFinished = false;
        }
    }
}


//***********************************************************************
bool ReadALine::getLine(char* result, unsigned resultSize, LineInfo& lineInfo, bool isContinue) {
//***********************************************************************
    if (!is_open || fp == nullptr || resultSize == 0)
        return false;
    result[0] = 0;
    if (pInclude != nullptr) { // all branches are finished with return
        if (!pInclude->getLine(result, resultSize, lineInfo)) {
            delete pInclude;
            pInclude = nullptr;
            // nothing usable in the inculde file, continue reading this file
            return getLine(result, resultSize, lineInfo);
        }
        else {
            if (pInclude->isEOF()) {
                delete pInclude;
                pInclude = nullptr;
                if (result[0] == 0)
                    return getLine(result, resultSize, lineInfo);
                return true;
            }
            return true;
        }
    }
    bool isNotFinished = true;
    bool inQuote = false;
    actLine++;
    enum States{ stStart, stEOF, stEOL, stCommentLine, stNormalRead, stSpace, stIncludeCheck };
    States st = isContinue ? stNormalRead :stStart;
    const char* includeString = ".INCLUDE";
    unsigned resultIndex = 0, includeIndex = 0;
    while (isNotFinished) {
        int ch = getChar();
        if (ch == EOF)
            st = stEOF;
        else if (ch == '\n')
            st = stEOL;
        if (ch == '\"')
            inQuote = !inQuote;
        if (!inQuote)
            ch = toupper(ch);
        switch (st) {
            case stStart:
                if (isspace(ch) || ch == '*' || ch == ';')
                    st = stCommentLine;
                else if (ch == '/' && peekChar() == '/')
                    st = stCommentLine;
                else if (ch == '+')
                    throw hmgExcept("ReadALine::getLine", "unexpected starting + in line %u of %s", actLine, names[fileNameIndex].name.c_str());
                else if (ch == '.') {
                    st = stIncludeCheck;
                    includeIndex = 1;
                }
                else {
                    result[resultIndex++] = char(ch);
                    lineInfo.firstLine = actLine;
                    lineInfo.sourceIndex = fileNameIndex;
                    st = stNormalRead;
                }
                break;
            case stCommentLine: // ignores everything in the line
                break;
            case stIncludeCheck:
                if (includeIndex == 8) {
                    if (isspace(ch)) {
                        if (pInclude != nullptr)
                            throw hmgExcept("ReadALine::getLine", "program error: include when there is open include file (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                        std::string includeFileNameAndPath;
                        int cc;
                        // ignores leading whitespaces
                        while (isspace(cc = peekChar()) && cc != EOF && cc != '\n' && cc != '\r')
                            getChar();
                        while ((cc = getChar()) != EOF && cc != '\n' && cc != '\r')
                            includeFileNameAndPath.push_back(char(cc));
                        if (includeFileNameAndPath.length() == 0)
                            throw hmgExcept("ReadALine::getLine", "missing .INCLUDE filename (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                        std::string newPath, newName;
                        filePathAndNameCreator(names[fileNameIndex].path, includeFileNameAndPath, newPath, newName);
                        pInclude = new ReadALine;
                        if (!pInclude->open(newPath, newName))
                            throw hmgExcept("ReadALine::getLine", "cannot open include file %s (line %u of %s)", includeFileNameAndPath.c_str(), actLine, names[fileNameIndex].name.c_str());
                        actLine--;
                        return getLine(result, resultSize, lineInfo); // will read from the include
                    }
                    // else jump forward
                }
                else if (includeString[includeIndex++] == ch)
                    break;
                else
                    includeIndex--; // the last character was different, should not copy
                // we can be here in two ways: if !isspace after .INCLUDE or includeString[includeIndex++] != ch
                if (resultIndex + includeIndex >= resultSize)
                    throw hmgExcept("ReadALine::getLine", "line is too long (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                for(unsigned i=0; i< includeIndex; i++)
                    result[resultIndex++] = includeString[i];
                lineInfo.firstLine = actLine;
                lineInfo.sourceIndex = fileNameIndex;
                st = stNormalRead;
                // !!!! intentionally there is no break here because case stNormalRead will process ch
            case stNormalRead:
label_NR:
                if (isspace(ch))
                    st = stSpace;
                else if (ch == ';')
                    st = stCommentLine;
                else if (ch == '/' && peekChar() == '/')
                    st = stCommentLine;
                else {
                    if (resultIndex + 1 >= resultSize)
                        throw hmgExcept("ReadALine::getLine", "line is too long (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                    result[resultIndex++] = char(ch);
                }
                break;
            case stSpace:
                if (isspace(ch))
                    break;
                if (resultIndex + 2 >= resultSize)
                    throw hmgExcept("ReadALine::getLine", "line is too long (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                result[resultIndex++] = ' ';
                st = stNormalRead;
                goto label_NR;
                break;
        }
        if (st == stEOL) {
            if(peekChar()=='\r') // if the line is finished with \n\r
                getChar();
            if (resultIndex > 0) {
                result[resultIndex] = 0;
                lineInfo.lastLine = actLine;
                appendLine(result, resultSize, lineInfo);
                bool isIdentical = true;
                for (unsigned i = 0; isIdentical && i < 8; i++)
                    if (result[i] != includeString[i])
                        isIdentical = false;
                if(isIdentical)
                    throw hmgExcept("ReadALine::getLine", ".INCLUDE line cannot be broken (line %u of %s)", actLine, names[fileNameIndex].name.c_str());
                isNotFinished = false;
            }
            else {
                actLine++; // the function starts with actLine++, so only if we read more than one line in the loop
                st = stStart;
            }
        }
        if (st == stEOF) {
            if (resultIndex == 0)
                return false;
            result[resultIndex] = 0;
            lineInfo.lastLine = actLine;
            isNotFinished = false;
        }
    }
    if (resultIndex > 0 && result[resultIndex - 1] == ' ')
        result[resultIndex - 1] = 0;
    return true;
}


//***********************************************************************
bool GlobalHMGFileNames::textToDeepInterfaceNodeID(char* token, uns fullCircuitIndex, DeepInterfaceNodeID& dest) {
//***********************************************************************
    dest.componentID.clear();
    if (strcmp(token, "0") == 0) {
        dest.nodeID.index = unsMax;
        return true;
    }
    uns componentIndex = 0;
    HMGFileModelDescription* currentComponent = modelData[fullCircuitData[fullCircuitIndex]->modelID];
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            if (currentComponent == nullptr)
                return false;
            uns ci = currentComponent->instanceListIndex[token];
            dest.componentID.push_back(ci);
            HMGFileComponentInstanceLine* pxline = currentComponent->instanceList[ci];
            currentComponent = pxline->isBuiltIn ? nullptr : modelData[pxline->modelIndex];
            token[i] = '.';
            componentIndex++;
            token += i + 1;
        }
        else {
            if (!textToSimpleInterfaceNodeID(token, dest.nodeID))
                return false;
            return true;
        }
    }
}


//***********************************************************************
bool GlobalHMGFileNames::textToDeepInterfaceVarID(char* token, DeepInterfaceNodeID& dest) {
// dest constains the full circuit ID
//***********************************************************************
    dest.componentID.clear();
    if (strcmp(token, "0") == 0) {
        dest.nodeID.index = unsMax;
        return true;
    }
    uns componentIndex = 0;
    HMGFileModelDescription* currentComponent = nullptr;
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            if (currentComponent == nullptr) {
                if (!dest.componentID.empty() || !fullCircuitNames.contains(token))
                    return false;
                uns fullCircuitIndex = fullCircuitNames[token];
                dest.componentID.push_back(fullCircuitIndex);
                currentComponent = modelData[fullCircuitData[fullCircuitIndex]->modelID];
            }
            else {
                uns ci = currentComponent->instanceListIndex[token];
                dest.componentID.push_back(ci);
                HMGFileComponentInstanceLine* pxline = currentComponent->instanceList[ci];
                currentComponent = pxline->isBuiltIn ? nullptr : modelData[pxline->modelIndex];
            }
            token[i] = '.';
            componentIndex++;
            token += i + 1;
        }
        else {
            if (!textToSimpleInterfaceNodeID(token, dest.nodeID))
                return false;
            return true;
        }
    }
}


//***********************************************************************
bool GlobalHMGFileNames::textToDeepCDNodeID(char* token, uns fullCircuitIndex, DeepCDNodeID& dest) {
//***********************************************************************
    dest.componentID.clear();
    if (strcmp(token, "0") == 0) {
        dest.nodeID.index = unsMax;
        return true;
    }
    uns componentIndex = 0;
    HMGFileModelDescription* currentComponent = modelData[fullCircuitData[fullCircuitIndex]->modelID];
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            if (currentComponent == nullptr)
                return false;
            uns ci = currentComponent->instanceListIndex[token];
            dest.componentID.push_back(ci);
            HMGFileComponentInstanceLine* pxline = currentComponent->instanceList[ci];
            currentComponent = pxline->isBuiltIn ? nullptr : modelData[pxline->modelIndex];
            token[i] = '.';
            componentIndex++;
            token += i + 1;
        }
        else {
            if (!textToCDNode(token, dest.nodeID))
                return false;
            return true;
        }
    }
}


//***********************************************************************
bool GlobalHMGFileNames::textRawToDeepInterfaceNodeID(char* token, DeepInterfaceNodeID& dest) {
//***********************************************************************
    dest.componentID.clear();
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            uns ci = 0;
            if (sscanf_s(token, "%u", &ci) != 1)
                return false;
            dest.componentID.push_back(ci);
            token[i] = '.';
            token += i + 1;
        }
        else {
            if (!textToSimpleInterfaceNodeID(token, dest.nodeID))
                return false;
            return true;
        }
    }
}


//***********************************************************************
bool GlobalHMGFileNames::textRawToDeepCDNodeID(char* token, DeepCDNodeID& dest) {
//***********************************************************************
    dest.componentID.clear();
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            uns ci = 0;
            if (sscanf_s(token, "%u", &ci) != 1)
                return false;
            dest.componentID.push_back(ci);
            token[i] = '.';
            token += i + 1;
        }
        else {
            if (!textToCDNode(token, dest.nodeID))
                return false;
            return true;
        }
    }
}


//***********************************************************************
void HmgFileReader::ReadFile(const std::string& fileNameWithPath) {
//***********************************************************************
    char line[MAX_LINE_LENGHT];
    LineInfo lineInfo;
    ReadALine reader;
    std::string newPath, newName, path0;

    // open .hmg and check it is a HEXMG file and version is OK.

    filePathAndNameCreator(path0, fileNameWithPath, newPath, newName);
    if(!reader.open(newPath, newName))
        throw hmgExcept("HmgFileReader::ReadFile", "cannot open %s", fileNameWithPath.c_str());

    if(!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
        throw hmgExcept("HmgFileReader::ReadFile", "cannot read first line from %s", fileNameWithPath.c_str());

    LineTokenizer lineToken;
    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (strcmp(token, "HEXMG") != 0)
        throw hmgExcept("HmgFileReader::ReadFile", "The .hmg file must start with HEXMG, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    uns mainVersion, subVersion;
    if (sscanf(token, "%u.%u", &mainVersion, &subVersion) != 2)
        throw hmgExcept("HmgFileReader::ReadFile", "The first line must be HEXMG mainversion.subversion, HEXMG %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (mainVersion > 1 || subVersion > 0)
        throw hmgExcept("HmgFileReader::ReadFile", "Supported .hmg version is <= 1.0, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // reading the simulation

    globalCircuit.Read(reader, line, lineInfo);
    most("Read");
    //globalCircuit.ProcessXLines();
    //most("ProcessXLines");
    //globalCircuit.ProcessExpressionNames(&globalCircuit.itemList);
    //most("ProcessExpressionNames");
}


//***********************************************************************
void HMGFileModelDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read subcircuit head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".MODEL") != 0)
        throw hmgExcept("HMGFileModelDescription::Read", ".MODEL expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read model name

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    fullName = lineToken.getActToken();

    if (globalNames.modelNames.contains(fullName))
        throw hmgExcept("HMGFileModelDescription::Read", ".MODEL %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    modelIndex = (uns)globalNames.modelNames.size();
    globalNames.modelNames[fullName] = modelIndex;
    vectorForcedSet(globalNames.modelData, this, modelIndex);

    // read model type

    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileModelDescription::Read", ".MODEL type expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if(strcmp(lineToken.getActToken(), "SUBCIRCUIT") == 0)
        modelType = hfmtSubcircuit;
    else if (strcmp(lineToken.getActToken(), "CONTROLLER") == 0)
        modelType = hfmtController;
    else
        throw hmgExcept("HMGFileModelDescription::Read", "unknown model type, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);


    // read node, variable and parameter numbers

    externalNs.zero();
    internalNs.zero();

    if (!lineToken.isSepEOL && !lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileModelDescription::Read", "node/parameter=number expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    while (!lineToken.isSepEOL) {
        if      (readNodeOrParNumber(line, lineToken, reader, lineInfo, "X", externalNs.nIONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "N", internalNs.nNormalInternalNodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "P", externalNs.nParams));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "IN", externalNs.nNormalINodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "CIN", externalNs.nControlINodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "OUT", externalNs.nNormalONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "FWOUT", externalNs.nForwardedONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "C", internalNs.nControlInternalNodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "V", internalNs.nInternalVars));
        else if (strcmp(lineToken.getActToken(), "SUNRED") == 0) {
            solutionType = stSunRed;
            lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            solutionDescriptionIndex = globalNames.sunredTreeNames.at(lineToken.getActToken());
        }
        else
            throw hmgExcept("HMGFileModelDescription::Read", "unknown node/parameter type, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if(!lineToken.isSepEOL && !lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
            throw hmgExcept("HMGFileModelDescription::Read", "simple node name expected, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }

    if (modelType == hfmtSubcircuit)
        ReadOrReplaceBodySubcircuit(reader, line, lineInfo);
    else if (modelType == hfmtController)
        ReadOrReplaceBodyController(reader, line, lineInfo);
    else
        throw hmgExcept("HMGFileModelDescription::Read", "Internal error: unknown model type (%u)", modelType);
}


//***********************************************************************
void HMGFileModelDescription::Replace(HMGFileModelDescription* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    modelIndex = pParent->modelIndex;
    fullName = parent->fullName;
    if (modelType == hfmtSubcircuit)
        ReadOrReplaceBodySubcircuit(reader, line, lineInfo);
    else if (modelType == hfmtController)
        ReadOrReplaceBodyController(reader, line, lineInfo);
    else
        throw hmgExcept("HMGFileModelDescription::Read", "Internal error: unknown model type (%u)", modelType);
}


//***********************************************************************
void HMGFileModelDescription::ReadOrReplaceBodySubcircuit(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    bool isModelNotEnded = true;
    LineTokenizer lineToken;

    do {
        if(!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "incomplete .MODEL definition (%s), missing .END in %s", fullName.c_str(), reader.getFileName(lineInfo).c_str());
        if (line[0] == '.') {
            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, ".END") == 0) {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (strcmp(token, "MODEL") != 0)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".END MODEL expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (fullName != token)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".END MODEL %s expected, %s arrived in %s, line %u", fullName.c_str(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                isModelNotEnded = false;
            }
            else if (strcmp(token, ".MODEL") == 0) {
                throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".MODEL in .MODEL not allowed in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }
            else if (strcmp(token, ".DEFAULTRAIL") == 0) {
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                SimpleInterfaceNodeID rail;
                if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                SimpleInterfaceNodeID node;
                while (!lineToken.isSepEOL) {
                    lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), node))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (!checkNodeValidity(node))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: invalid node index: %u in %s, line %u: %s", node.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (defaults.size() > 0 && defaults.back().rail == rail.index && defaults.back().type == node.type) {
                        auto& last = defaults.back();
                        if (node.index == last.start_index - 1)
                            last.start_index--;
                        else if (node.index == last.stop_index + 1)
                            last.stop_index++;
                        else
                            defaults.push_back({ rail.index, node.type, node.index, node.index });
                    }
                    else
                        defaults.push_back({ rail.index, node.type, node.index, node.index });
                }
            }
            else if (strcmp(token, ".DEFAULTRAILRANGE") == 0) {
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                SimpleInterfaceNodeID rail;
                if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                SimpleInterfaceNodeID node1, node2;
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), node1))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), node2))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if (!checkNodeValidity(node1))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: invalid node index: %u in %s, line %u: %s", node1.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if (!checkNodeValidity(node2))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: invalid node index: %u in %s, line %u: %s", node2.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if(node1.type != node2.type)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: node range required in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                defaults.push_back({ rail.index, node1.type, node1.index < node2.index ? node1.index : node2.index, node1.index < node2.index ? node2.index : node1.index });
            }
            else
                throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else {
            HMGFileComponentInstanceLine* pxline = new HMGFileComponentInstanceLine;
            instanceList.push_back(pxline);

            lineToken.init(line);

            // component name

            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            std::string instanceName = token;
            pxline->isController = false;

            // component type

            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

            pxline->modelIndex = unsMax;

            uns nodenum = 2, parnum = 1, funcnum = 0;
            uns startONodes = unsMax, stopONodes = unsMax;
            if (strcmp(token, "MODEL") == 0) {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                pxline->isBuiltIn = false;
                try { pxline->modelIndex = globalNames.modelNames.at(token); }
                catch (const std::out_of_range&) {
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised MODEL (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                }
                const HMGFileModelDescription& mod = *globalNames.modelData[pxline->modelIndex];
                nodenum = mod.externalNs.nIONodes + mod.externalNs.nNormalINodes + mod.externalNs.nControlINodes + mod.externalNs.nNormalONodes + mod.externalNs.nForwardedONodes;
                parnum = mod.externalNs.nParams;
                pxline->isController = mod.modelType == hfmtController;
                if (mod.externalNs.nNormalONodes != 0) {
                    startONodes = mod.externalNs.nIONodes + mod.externalNs.nNormalINodes + mod.externalNs.nControlINodes;
                    stopONodes = startONodes + mod.externalNs.nNormalONodes - 1;
                }
            }
            else if (strcmp(token,  "R") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstR_1; }
            else if (strcmp(token, "R2") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstR_2; parnum = 2; }
            else if (strcmp(token,  "G") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstG_1; }
            else if (strcmp(token, "G2") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstG_2; parnum = 2; }
            else if (strcmp(token,  "C") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstC_1; }
            else if (strcmp(token, "C2") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstC_2; parnum = 2; }
            else if (strcmp(token,  "I") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstI_1; parnum = 4; }
            else if (strcmp(token, "I2") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstI_2; parnum = 5; }
            else if (strcmp(token, "VI") == 0) { pxline->isBuiltIn = true; pxline->modelIndex = bimtConstVI;  parnum = 5; startONodes = 2; stopONodes = 2; }
            // don't forget to set pxline->isController if needed !
            // don't forget to set startONodes and stopONodes if there are normal O nodes !

            // read nodes and parameters
            
            if (pxline->modelIndex != unsMax) {
                for (uns i = 0; i < nodenum; i++) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    pxline->nodes.emplace_back(SimpleInterfaceNodeID());
                    if (!textToSimpleInterfaceNodeID(token, pxline->nodes.back()))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (pxline->nodes.back().type == nvtUnconnected) {
                        if(startONodes == unsMax || i < startONodes || i > stopONodes)
                            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "NONE node connection allowed only for normal output nodes (OUT) in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    }
                }
                for (uns i = 0; i < parnum; i++) {
                    pxline->params.emplace_back(ParameterInstance());
                }
                for (uns i = 0; i < parnum; i++) { // the actual number of parameters can be less than the formal parameter number, the missing parameters are 0 values
                    if (lineToken.isSepEOL)
                        break;
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (strcmp(token, "DEFAULTRAIL") == 0)
                        break;
                    if (pxline->modelIndex == bimtConstI_1 || pxline->modelIndex == bimtConstI_2) {
                        uns index = parnum;
                        if (     strcmp(token, "DC0") == 0) index = 0;
                        else if (strcmp(token, "DC")  == 0) index = 1;
                        else if (strcmp(token, "AC")  == 0) index = 2;
                        else if (strcmp(token, "PHI") == 0) index = 3;
                        else if (strcmp(token, "MUL") == 0) index = 4; // for bimtConstI_1 index = 4 == parnum => the error is handled in the next if-else section => it cannot be in an else branch
                        if (index < parnum) {
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (isalpha(token[0])) { // parameter / variable / node
                                if (!textToSimpleInterfaceNodeID(token, pxline->params[index].param))
                                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised parameter/variable/node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                            }
                            else { // value
                                if (!spiceTextToRvt(token, pxline->params[index].value))
                                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                            }
                            continue;
                        }
                    }
                    // this part cannot be put in else section ! (see above):
                    if (isalpha(token[0])) { // parameter / variable / node
                        if (!textToSimpleInterfaceNodeID(token, pxline->params[i].param))
                            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised parameter/variable/node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    }
                    else { // value
                        if (!spiceTextToRvt(token, pxline->params[i].value))
                            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    }
                }
                for (uns i = 0; i < funcnum; i++) {
                    TODO("Read function calls");
                }
            }
            else
                throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised component type (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

            if (!lineToken.isSepEOL) {
                bool isDefRail = (strcmp(token, "DEFAULTRAIL") == 0);
                if (!isDefRail) { // for e.g. current source the parameters can be less than the formal number of paramteres
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    isDefRail = (strcmp(token, "DEFAULTRAIL") == 0);
                }
                if (isDefRail) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    SimpleInterfaceNodeID rail;
                    if (!textToSimpleInterfaceNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "DEFAULTRAIL: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    pxline->isDefaultRail = true;
                    pxline->defaultValueRailIndex = rail.index;
                }
                else
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised line ending (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }

            // controller instance or node instance => setting instance index

            if (pxline->isController) {
                if (controllerInstanceNameIndex.contains(instanceName))
                    throw hmgExcept("HMGFileModelDescription::Read", "%s redefinition in %s, line %u: %s", instanceName.c_str(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                pxline->instanceIndex = (uns)controllerInstanceNameIndex.size();
                controllerInstanceNameIndex[instanceName] = pxline->instanceIndex;
            }
            else {
                if (componentInstanceNameIndex.contains(instanceName))
                    throw hmgExcept("HMGFileModelDescription::Read", "%s redefinition in %s, line %u: %s", instanceName.c_str(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                pxline->instanceIndex = (uns)componentInstanceNameIndex.size();
                componentInstanceNameIndex[instanceName] = pxline->instanceIndex;
            }
            instanceListIndex[instanceName] = (uns)instanceListIndex.size();
        }
    } while (isModelNotEnded);
}


//***********************************************************************
void HMGFileRails::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read Rails head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".RAILS") != 0)
        throw hmgExcept("HMGFileRails::Read", ".RAILS expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read rails number (optional)

    if (lineToken.isSepEOL)
        throw hmgExcept("HMGFileRails::Read", "empty .RALIS line: %s in %s, line %u", line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (lineToken.isSepAssignment) {
        if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
            throw hmgExcept("HMGFileRails::Read", ".RALIS=number expected, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (sscanf_s(lineToken.getActToken(), "%u", &nRails) != 1)
            throw hmgExcept("HMGFileRails::Read", ".RALIS=number is not a number, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (nRails > maxRails)
            throw hmgExcept("HMGFileRails::Read", ".RALIS=number must be <= %u, %s arrived (%s) in %s, line %u", maxRails, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        globalNRails = nRails;
    }

    // read rail values

    while (!lineToken.isSepEOL) {
        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        SimpleInterfaceNodeID nid;
        if (!textToSimpleInterfaceNodeID(token, nid))
            throw hmgExcept("HMGFileRails::Read", "unrecognised rail (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        if (nid.type != nvtRail)
            throw hmgExcept("HMGFileRails::Read", "not a rail (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        uns railIndex = nid.index;
        if(railIndex > globalNRails)
            throw hmgExcept("HMGFileRails::Read", "rail index must be <= the number of rails (%u), %s arrived (%s) in %s, line %u", globalNRails, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        
        rvt value = rvt0;
        uns position = 0;
        lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (!spiceTextToRvt(lineToken.getActToken(), position, value))
            throw hmgExcept("HMGFileRails::Read", "R%u=rail value expected, %s arrived (%s) in %s, line %u", railIndex, lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        railValues.push_back(std::pair<uns, rvt>(railIndex, value));
    }
}


//***********************************************************************
void HMGFileCreate::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read Create head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".CREATE") != 0)
        throw hmgExcept("HMGFileCreate::Read", ".CREATE expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // FullCircuit name

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (globalNames.fullCircuitNames.contains(token))
        throw hmgExcept("HMGFileCreate::Read", ".CREATE %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    fullCircuitIndex = (uns)globalNames.fullCircuitNames.size();
    globalNames.fullCircuitNames[token] = fullCircuitIndex;
    vectorForcedSet(globalNames.fullCircuitData, this, fullCircuitIndex);

    // model ID

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    try { modelID = globalNames.modelNames.at(token); }
    catch (const std::out_of_range&) {
        throw hmgExcept("HMGFileCreate::Read", "unrecognised MODEL (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }
    const HMGFileModelDescription& mod = *globalNames.modelData[modelID];
    if (mod.modelType != HMGFileModelDescription::hfmtSubcircuit)
        throw hmgExcept("HMGFileCreate::Read", "the created model (%s) must be SUBCIRCUIT in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

    // GND

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (strcmp(token, "GND") != 0)
        throw hmgExcept("HMGFileCreate::Read", "GND expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    SimpleInterfaceNodeID nid;
    if (!textToSimpleInterfaceNodeID(token, nid))
        throw hmgExcept("HMGFileCreate::Read", "unrecognised rail (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    if(nid.type != nvtRail)
        throw hmgExcept("HMGFileCreate::Read", "not a rail (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    GND = nid.index;
}


//***********************************************************************
void HMGFileProbe::Read(ReadALine& reader, char* line, LineInfo& lineInfo, bool isContinue) {
//***********************************************************************
    LineTokenizer lineToken;

    // read probe head

    lineToken.init(line);
    char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".PROBE") != 0)
        throw hmgExcept("HMGFileCreate::Read", ".PROBE expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // probe name

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if(globalNames.probeNames.contains(token) != isContinue)
        throw hmgExcept("HMGFileCreate::Read", "Program error: globalNames.probeNames.contains(token) != isContinue");
    if (!isContinue) {
        probeIndex = (uns)globalNames.probeNames.size();
        globalNames.probeNames[token] = probeIndex;
        vectorForcedSet(globalNames.probeData, this, probeIndex);

        // probe type

        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

        if (     strcmp(token, "V"       ) == 0) probeType = ptV;
        else if (strcmp(token, "I"       ) == 0) probeType = ptI;
        else if (strcmp(token, "VSUM"    ) == 0) probeType = ptVSum;
        else if (strcmp(token, "VAVERAGE") == 0) probeType = ptVAverage;
        else if (strcmp(token, "ISUM"    ) == 0) probeType = ptISum;
        else if (strcmp(token, "IAVERAGE") == 0) probeType = ptIAverage;
        else
            throw hmgExcept("HMGFileCreate::Read", "Unknown probe type (%s) in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

        // fullCircuit

        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        try { fullCircuitID = globalNames.fullCircuitNames.at(token); }
        catch (const std::out_of_range&) {
            throw hmgExcept("HMGFileCreate::Read", "unrecognised full circuit (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
    }
    else {

        // probe type

        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (strcmp(token, "V") == 0) {
            if(probeType != ptV)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "I") == 0) {
            if (probeType != ptI)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "VSUM") == 0) {
            if (probeType != ptVSum)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "VAVERAGE") == 0) {
            if (probeType != ptVAverage)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "ISUM") == 0) {
            if (probeType != ptISum)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "IAVERAGE") == 0) {
            if (probeType != ptIAverage)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else
            throw hmgExcept("HMGFileCreate::Read", "Unknown probe type (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

        // fullCircuit

        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        uns newFullCircID;
        try { newFullCircID = globalNames.fullCircuitNames.at(token); }
        catch (const std::out_of_range&) {
            throw hmgExcept("HMGFileCreate::Read", "unrecognised MODEL (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        if (newFullCircID != fullCircuitID)
            throw hmgExcept("HMGFileCreate::Read", "A different full circuit ID than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }

    while (!lineToken.isSepEOL) {
        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        DeepInterfaceNodeID pnid;
        if(!globalNames.textToDeepInterfaceNodeID(token, fullCircuitID, pnid))
            throw hmgExcept("HMGFileCreate::Read", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        nodes.push_back(pnid);
    }
}


//***********************************************************************
void HMGFileRun::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read Run head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".RUN") != 0)
        throw hmgExcept("HMGFileRun::Read", ".RUN expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // fullCircuitID

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (globalNames.fullCircuitNames.contains(token)) {
        data.fullCircuitID = globalNames.fullCircuitNames[token];
        data.isMultigrid = false;
    }
    else if (globalNames.multigridNames.contains(token)) {
        data.fullCircuitID = globalNames.multigridNames[token];
        data.isMultigrid = true;
    }
    else
        throw hmgExcept("HMGFileRun::Read", "unrecognised full circuit name or multigrid name (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

    // analysisType

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (     strcmp(token, "DC")        == 0)   data.analysisType = atDC;
    else if (strcmp(token, "TIMESTEP")  == 0)   data.analysisType = atTimeStep;
    else if (strcmp(token, "AC")        == 0)   data.analysisType = atAC;
    else if (strcmp(token, "TIMECONST") == 0) { data.analysisType = atTimeConst; data.iterNumSPD = 5; }
    else
        throw hmgExcept("HMGFileRun::Read", "unrecognised analysis type (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

    while (!lineToken.isSepEOL) {
        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (strcmp(token, "INITIAL") == 0) data.isInitial = true;
        else if (strcmp(token, "ITER") == 0) data.iterNumSPD = 1;
        else if (strcmp(token, "ITERS") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if(sscanf_s(token, "%u", &data.iterNumSPD) != 1)
                throw hmgExcept("HMGFileRun::Read", "unrecognised ITERS number (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "PRE") == 0) data.isPre = true;
        else if (strcmp(token, "ERR") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!spiceTextToRvt(token, data.err))
                throw hmgExcept("HMGFileRun::Read", "unrecognised ERR value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "T") == 0) {
            data.isDT = false;
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!spiceTextToRvt(token, data.fTauDtT))
                throw hmgExcept("HMGFileRun::Read", "unrecognised T value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "DT") == 0) {
            data.isDT = true;
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!spiceTextToRvt(token, data.fTauDtT))
                throw hmgExcept("HMGFileRun::Read", "unrecognised DT value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "TAU") == 0) {
            data.isTau = true;
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!spiceTextToRvt(token, data.fTauDtT))
                throw hmgExcept("HMGFileRun::Read", "unrecognised TAU value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "F") == 0) {
            data.isTau = false;
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!spiceTextToRvt(token, data.fTauDtT))
                throw hmgExcept("HMGFileRun::Read", "unrecognised F value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "SPD") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (sscanf_s(token, "%u", &data.iterNumSPD) != 1)
                throw hmgExcept("HMGFileRun::Read", "unrecognised SPD number (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else
            throw hmgExcept("HMGFileRun::Read", "Unknown RUN specifier (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }
}


//***********************************************************************
void HMGFileSave::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read Run head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".SAVE") != 0)
        throw hmgExcept("HMGFileSave::Read", ".RUN expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // save types and filename

    while (true) {
        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (strcmp(token, "RAW") == 0) isRaw = true;
        else if (strcmp(token, "APPEND") == 0) isAppend = true;
        else if (strcmp(token, "MAXRESULTSPERROW") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (sscanf_s(token, "%u", &maxResultsPerRow) != 1)
                throw hmgExcept("HMGFileSave::Read", "unrecognised MAXRESULTSPERROW number (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "FILE") == 0) {
            if(!lineToken.getQuotedText())
                throw hmgExcept("HMGFileSave::Read", "cannot find filename in \"\" in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            fileName = token; // token points to lineToken.token
            break;
        }
        else
            throw hmgExcept("HMGFileSave::Read", "unrecognised parameter name (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        if(lineToken.isSepEOL)
            throw hmgExcept("HMGFileSave::Read", "cannot find FILE= in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    }

    // probes

    while (!lineToken.isSepEOL) {
        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        probeIDs.push_back(globalNames.probeNames.at(token));
    }
}


//***********************************************************************
void HMGFileSet::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read Set head

    lineToken.init(line);
    char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".SET") != 0)
        throw hmgExcept("HMGFileSet::Read", ".SET expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // var ID

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!globalNames.textToDeepInterfaceVarID(token, varID))
        throw hmgExcept("HMGFileSet::Read", "unrecognised variable (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    if (varID.nodeID.type != nvtVarInternal && varID.nodeID.type != nvtVarGlobal)
        throw hmgExcept("HMGFileSet::Read", "not a variable (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

    // value

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!spiceTextToRvt(token, value))
        throw hmgExcept("HMGFileSet::Read", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
}


//***********************************************************************
void HMGFileSunredTree::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read SunredTree head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".SUNREDTREE") != 0)
        throw hmgExcept("HMGFileSunredTree::Read", ".SUNREDTREE expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read tree name

    if (lineToken.isSepEOL || lineToken.isSepOpeningBracket)
        throw hmgExcept("HMGFileSunredTree::Read", "missing .SUNREDTREE name in %s, line %u", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileSunredTree::Read", "simple .SUNREDTREE name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    
    fullName = lineToken.getActToken();
    
    if (globalNames.sunredTreeNames.contains(fullName))
        throw hmgExcept("HMGFileSunredTree::Read", ".SUNREDTREE %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    sunredTreeIndex = (uns)globalNames.sunredTreeNames.size();
    globalNames.sunredTreeNames[fullName] = sunredTreeIndex;

    ReadOrReplaceBody(reader, line, lineInfo);
}


//***********************************************************************
void HMGFileSunredTree::Replace(HMGFileSunredTree* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    sunredTreeIndex = pParent->sunredTreeIndex;
    fullName = parent->fullName;
    ReadOrReplaceBody(reader, line, lineInfo);
}


//***********************************************************************
void HMGFileSunredTree::ReadOrReplaceBody(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    bool isSunredTreeNotEnded = true;
    LineTokenizer lineToken;
    std::set<CellID> srcCheck, destCheck;

    do {
        if(!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "incomplete .SUNREDTREE definition, missing .END in %s", reader.getFileName(lineInfo).c_str());
        if (line[0] != '.') {
            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, "RED") == 0) {
                CellID dest, src1, src2;

                // read

                if(sscanf(line, "%*s %u %u %u %u %u %u", &dest.level, &dest.index, &src1.level, &src1.index, &src2.level, &src2.index) != 6)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "wrong RED format, %s arrived in %s, line %u", 
                        line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                
                // error check
                
                if(!srcCheck.insert(src1).second)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "SUNRED cell reduced more than once (src1): %s in %s, line %u",
                        line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!srcCheck.insert(src2).second)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "SUNRED cell reduced more than once (src2): %s in %s, line %u",
                        line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!destCheck.insert(dest).second)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "calculation of a SUNRED cell is allowed only once, here the destination cell is repeated: %s in %s, line %u",
                        line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if( dest.level == 0)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "destination level cannot be 0: %s in %s, line %u",
                        line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

                // store

                if (reductions.size() < dest.level)
                    reductions.resize(dest.level);
                vectorForcedSet(reductions[dest.level - 1], { src1.level, src1.index, src2.level, src2.index, true }, dest.index);
            }
            else
                throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "unknown .SUNREDTREE instruction or missing .END, %s arrived in %s, line %u", 
                    line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        }
        else {
            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, ".END") == 0) {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if(strcmp(token, "SUNREDTREE") != 0)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", ".END SUNREDTREE expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if(fullName != token)
                    throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", ".END SUNREDTREE %s expected, %s arrived in %s, line %u: %s", fullName.c_str(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                for (size_t i = 0; i < reductions.size(); i++)
                    for (size_t j = 0; j < reductions[i].size(); j++)
                        if (!reductions[i][j].isValid)
                            throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "SUNREDTREE %s: undefined destination cell [level = %u, index = %u]", fullName.c_str(), (uns)(i + 1), (uns)j);
                isSunredTreeNotEnded = false;
            }
            else
                throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
    } while (isSunredTreeNotEnded);
}


//***********************************************************************
void HMGFileMultiGrid::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read MultiGrid head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".MULTIGRID") != 0)
        throw hmgExcept("HMGFileMultiGrid::Read", ".MULTIGRID expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read MultiGrid name

    if (lineToken.isSepEOL || lineToken.isSepOpeningBracket)
        throw hmgExcept("HMGFileMultiGrid::Read", "missing .MULTIGRID name in %s, line %u", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileMultiGrid::Read", "simple .MULTIGRID name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    
    fullName = lineToken.getActToken();
    
    if (globalNames.multigridNames.contains(fullName))
        throw hmgExcept("HMGFileMultiGrid::Read", ".MULTIGRID %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    multigridIndex = (uns)globalNames.multigridNames.size();
    globalNames.multigridNames[fullName] = multigridIndex;

    ReadOrReplaceBody(reader, line, lineInfo);
}


//***********************************************************************
void HMGFileMultiGrid::Replace(HMGFileMultiGrid* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    multigridIndex = pParent->multigridIndex;
    fullName = parent->fullName;
    ReadOrReplaceBody(reader, line, lineInfo);
}


//***********************************************************************
void HMGFileMultiGrid::ReadOrReplaceBody(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    bool isMultigridNotEnded = true;
    LineTokenizer lineToken;

    do {
        if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "incomplete .SUNREDTREE definition, missing .END in %s", reader.getFileName(lineInfo).c_str());
        lineToken.init(line);
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        uns code = 0;
        if (strcmp(token, ".LEVEL") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            uns lvl = 0;
            if(sscanf_s(token, "%u", &lvl) != 1)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".LEVEL number expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            if (lvl > maxRails || lvl == 0)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".LEVEL number must be >0 and < %u, %s arrived in %s, line %u: %s", maxRails, token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            if (levels.size() < lvl)
                levels.resize(lvl);
            InterfaceFineCoarseConnectionDescription& level = levels[lvl - 1];

            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, "COARSE") != 0)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".LEVEL %u must be followed COARSE=, %s arrived in %s, line %u: %s", lvl, token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            try { level.indexCoarseFullCircuit = globalNames.fullCircuitNames.at(token); }
            catch (const std::out_of_range&) {
                throw hmgExcept("HMGFileCreate::Read", "unrecognised full circuit (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }

            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, "FINE") != 0)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".LEVEL %u COARSE=<fullcircuit name> must be followed FINE=, %s arrived in %s, line %u: %s", lvl, token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            try { level.indexFineFullCircuit = globalNames.fullCircuitNames.at(token); }
            catch (const std::out_of_range&) {
                throw hmgExcept("HMGFileCreate::Read", "unrecognised full circuit (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }
            HMGFileModelDescription* coarseFullCircuit = globalNames.modelData[globalNames.fullCircuitData[level.indexCoarseFullCircuit]->modelID];
            HMGFileModelDescription* fineFullCircuit = globalNames.modelData[globalNames.fullCircuitData[level.indexFineFullCircuit]->modelID];

            bool isLevelNotEnded = true;
            do {
                if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "incomplete .LEVEL definition, missing .END in %s", reader.getFileName(lineInfo).c_str());
                lineToken.init(line);
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

                if (strcmpC(token, ".GLOBALRESTRICTION", 1, code) == 0 || strcmpC(token, ".GLOBALPROLONGATION", 2, code) == 0) {
                    if (code == 1)  level.globalNodeRestrictions.emplace_back(InterfaceNodeInstruction());
                    else            level.globalNodeProlongations.emplace_back(InterfaceNodeInstruction());
                    InterfaceNodeInstruction& instr = code == 1 ? level.globalNodeRestrictions.back() : level.globalNodeProlongations.back();
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!textToSimpleInterfaceNodeID(token, instr.nodeID))
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if(instr.nodeID.type != nvtNInternal && instr.nodeID.type != nvtCInternal)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "in .GLOBALRESTRICTION and .GLOBALPROLONGATION only internal node types (N and C) allowed, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    
                    bool isRestrictionProlongationNotEnded = true;
                    do {
                        if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "incomplete .%s definition, missing .END in %s", code == 1 ? "GLOBALRESTRICTION" : "GLOBALPROLONGATION", reader.getFileName(lineInfo).c_str());
                        lineToken.init(line);
                        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

                        if (strcmp(token, "SRC") == 0) {
                            DeepInterfaceNodeID pnid;
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (!globalNames.textToDeepInterfaceNodeID(token, code == 1 ? level.indexFineFullCircuit : level.indexCoarseFullCircuit, pnid))
                                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                            rvt weight;
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (!spiceTextToRvt(token, weight))
                                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised weight value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                            instr.instr.push_back({ pnid.componentID.size() == 0 ? unsMax : (uns)pnid.componentID[0], pnid.nodeID, weight });
                        }
                        else if (strcmp(token, ".END") == 0) {
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (strcmp(token, code == 1 ? "GLOBALRESTRICTION" : "GLOBALPROLONGATION") != 0)
                                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END %s expected, %s arrived in %s, line %u: %s", code == 1 ? "GLOBALRESTRICTION" : "GLOBALPROLONGATION", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            // I'm lazy, I don't check the node name
                            isRestrictionProlongationNotEnded = false;
                        }
                        else
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                    } while (isRestrictionProlongationNotEnded);
                }
                else if (strcmp(token, ".COMPONENTGROUP") == 0) {
                    level.componentGroups.emplace_back(ComponentGroup());
                    ComponentGroup& cg = level.componentGroups.back();
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (lineToken.isSepEOL && strcmp(token, "COPY") == 0) {
                        cg.isCopy = true;
                    }
                    else {
                        cg.isCopy = false;
                        
                        if (!globalNames.localRestrictionTypeNames.contains(token))
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unknown LOCALRESTRICTIONTYPE: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                        cg.localRestrictionIndex = globalNames.localRestrictionTypeNames[token];
                        
                        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                        
                        if (!globalNames.localProlongationTypeNames.contains(token))
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unknown LOCALPROLONGATIONTYPE: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                        cg.localProlongationIndex = globalNames.localProlongationTypeNames[token];
                    }

                    bool isComponentGroupNotEnded = true;
                    do {
                        if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "incomplete .COMPONENTGROUP definition, missing .END in %s", reader.getFileName(lineInfo).c_str());
                        lineToken.init(line);
                        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

                        if (strcmp(token, "FINE") == 0) {
                            while (!lineToken.isSepEOL) {
                                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                                if(!fineFullCircuit->instanceListIndex.contains(token))
                                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unknown component name: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                                
                                cg.fineCells.push_back(fineFullCircuit->instanceListIndex[token]);
                            }
                        }
                        else if (strcmp(token, "COARSE") == 0) {
                            while (!lineToken.isSepEOL) {
                                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                                if (!coarseFullCircuit->instanceListIndex.contains(token))
                                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unknown component name: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                                cg.coarseCells.push_back(coarseFullCircuit->instanceListIndex[token]);
                            }
                        }
                        else if (strcmp(token, ".END") == 0) {
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (strcmp(token, "COMPONENTGROUP") != 0)
                                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END COMPONENTGROUP expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                            isComponentGroupNotEnded = false;
                        }
                        else
                            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                    } while (isComponentGroupNotEnded);
                }
                else if (strcmp(token, ".END") == 0) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (strcmp(token, "LEVEL") != 0)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END LEVEL expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    uns tmp = unsMax;
                    if (sscanf_s(token, "%u", &tmp) != 1 || tmp != lvl)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END LEVEL %u expected, %s arrived in %s, line %u: %s", lvl, token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    isLevelNotEnded = false;
                }
                else
                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised token in .LEVEL group (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            } while (isLevelNotEnded);
        }
        else if (strcmpC(token, ".LOCALRESTRICTIONTYPE", 1, code) == 0 || strcmpC(token, ".LOCALPROLONGATIONTYPE", 2, code) == 0) {
            bool isRestrict = false;
            uns lrtIndex = 0;
            if (code == 1) {
                isRestrict = true;
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (globalNames.localRestrictionTypeNames.contains(token))
                    throw hmgExcept("HMGFileMultiGrid::Read", ".LOCALRESTRICTIONTYPE %s redefinition in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                lrtIndex = (uns)globalNames.localRestrictionTypeNames.size();
                globalNames.localRestrictionTypeNames[token] = lrtIndex;
                localNodeRestrictionTypes.emplace_back(LocalProlongationOrRestrictionInstructions());
            }
            else {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (globalNames.localProlongationTypeNames.contains(token))
                    throw hmgExcept("HMGFileMultiGrid::Read", ".LOCALPROLONGATIONTYPE %s redefinition in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                lrtIndex = (uns)globalNames.localProlongationTypeNames.size();
                globalNames.localProlongationTypeNames[token] = lrtIndex;
                localNodeProlongationTypes.emplace_back(LocalProlongationOrRestrictionInstructions());
            }

            LocalProlongationOrRestrictionInstructions& rest = isRestrict ? localNodeRestrictionTypes.back() : localNodeProlongationTypes.back();
            bool isRestrictionProlongationNotEnded = true;
            LocalProlongationOrRestrictionInstructions::RecursiveInstruction recursive;
            bool isDeep = false;
            do {
                if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "incomplete .%s definition, missing .END in %s", isRestrict ? "LOCALRESTRICTIONTYPE" : "LOCALPROLONGATIONTYPE", reader.getFileName(lineInfo).c_str());
                lineToken.init(line);
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

                bool isToPush = false;
                bool isDestNode = false;
                bool isSrcDest = false;
                if (strcmp(token, ".DESTNODE") == 0) {
                    isToPush = recursive.nodeID.nodeID.type != cdntNone;
                    isDestNode = true;
                }
                else if (strcmpC(token, "SRC", 1, code) == 0 || strcmpC(token, "DEST", 2, code) == 0) {
                    recursive.instr.emplace_back(LocalProlongationOrRestrictionInstructions::OneRecursiveInstruction());
                    recursive.instr.back().isDestLevel = code == 2;
                    isSrcDest = true;
                }
                else if (strcmp(token, ".END") == 0) {
                    isToPush = recursive.nodeID.nodeID.type != cdntNone;
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (strcmp(token, isRestrict ? "LOCALRESTRICTIONTYPE" : "LOCALPROLONGATIONTYPE") != 0)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END %s expected, %s arrived in %s, line %u: %s", isRestrict ? "LOCALRESTRICTIONTYPE" : "LOCALPROLONGATIONTYPE", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if(isRestrict && globalNames.localRestrictionTypeNames[token] != lrtIndex)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END LOCALRESTRICTIONTYPE: incorrect identifier in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (!isRestrict && globalNames.localProlongationTypeNames[token] != lrtIndex)
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END LOCALPROLONGATIONTYPE: incorrect identifier in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    isRestrictionProlongationNotEnded = false;
                }
                else
                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

                if (isToPush) {
                    if (isDeep) {
                        rest.deepDestComponentNodes.push_back(std::move(recursive));
                    }
                    else {
                        rest.destComponentsNodes.emplace_back(LocalProlongationOrRestrictionInstructions::LocalNodeInstruction());
                        auto& dest = rest.destComponentsNodes.back();
                        dest.destIndex = recursive.nodeID.componentID[0];
                        dest.nodeID = recursive.nodeID.nodeID;
                        for (const auto& instr : recursive.instr) {
                            dest.instr.push_back({ instr.isDestLevel, instr.nodeID.componentID[0], instr.nodeID.nodeID, instr.weight });
                        }
                    }
                }
                if (isDestNode) {
                    recursive = LocalProlongationOrRestrictionInstructions::RecursiveInstruction();
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!globalNames.textRawToDeepCDNodeID(token, recursive.nodeID))
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    isDeep = recursive.nodeID.componentID.size() > 1;
                }
                if (isSrcDest) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!globalNames.textRawToDeepCDNodeID(token, recursive.instr.back().nodeID))
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    isDeep = isDeep || recursive.instr.back().nodeID.componentID.size() > 1;

                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!spiceTextToRvt(token, recursive.instr.back().weight))
                        throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                }

            } while (isRestrictionProlongationNotEnded);
        }
        else if (strcmp(token, ".END") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, "MULTIGRID") != 0)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END MULTIGRID expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (fullName != token)
                throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", ".END MULTIGRID %s expected, %s arrived in %s, line %u: %s", fullName.c_str(), token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            for (size_t i = 0; i < levels.size(); i++)
                if (levels[i].indexFineFullCircuit == unsMax)
                    throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "MULTIGRID %s: undefined level [level = %u]", fullName.c_str(), (uns)(i + 1));
            isMultigridNotEnded = false;
        }
        else
            throw hmgExcept("HMGFileMultiGrid::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    } while (isMultigridNotEnded);
}


//***********************************************************************
struct FileFunctionNameID {
//***********************************************************************
    const char* functionName = nullptr;
    fileFunctionType id = fftInvalid;
};


//***********************************************************************
FileFunctionNameID fftNameID[] = {
//***********************************************************************
    { "CONST",	fft_CONST },
    { "C_PI",	fft_C_PI },
    { "C_2PI",	fft_C_2PI },
    { "C_PI2",	fft_C_PI2 },
    { "C_E",	fft_C_E },
    { "C_T0",	fft_C_T0 },
    { "C_K",	fft_C_K },
    { "C_Q",	fft_C_Q },
    { "ADD",	fft_ADD },
    { "SUB",	fft_SUB },
    { "MUL",	fft_MUL },
    { "DIV",	fft_DIV },
    { "ADDC",	fft_ADDC },
    { "SUBC",	fft_SUBC },
    { "MULC",	fft_MULC },
    { "DIVC",	fft_DIVC },
    { "CADD",	fft_CADD },
    { "CSUB",	fft_CSUB },
    { "CMUL",	fft_CMUL },
    { "CDIV",	fft_CDIV },
    { "NEG",	fft_NEG },
    { "INV",	fft_INV },
    { "SQRT",	fft_SQRT },
    { "POW",	fft_POW },
    { "POWC",	fft_POWC },
    { "CPOW",	fft_CPOW },
    { "EXP",	fft_EXP },
    { "NEXP",	fft_NEXP },
    { "IEXP",	fft_IEXP },
    { "INEXP",	fft_INEXP },
    { "NIEXP",	fft_INEXP }, // !
    { "LN",	    fft_LN },
    { "LOG",	fft_LOG },
    { "CLOG",	fft_CLOG },
    { "ABS",	fft_ABS },
    { "ASIN",	fft_ASIN },
    { "ACOS",	fft_ACOS },
    { "ATAN",	fft_ATAN },
    { "ASINH",	fft_ASINH },
    { "ACOSH",	fft_ACOSH },
    { "ATANH",	fft_ATANH },
    { "SIN",	fft_SIN },
    { "COS",	fft_COS },
    { "TAN",	fft_TAN },
    { "SINH",	fft_SINH },
    { "COSH",	fft_COSH },
    { "TANH",	fft_TANH },
    { "RATIO",	fft_RATIO },
    { "PWL",	fft_PWL },
    { "DERIV",	fft_DERIV },
    { "DERIVC",	fft_DERIVC },
    { "VLENGTH2",	fft_VLENGTH2 },
    { "VLENGTH3",	fft_VLENGTH3 },
    { "DISTANCE2",	fft_DISTANCE2 },
    { "DISTANCE3",	fft_DISTANCE3 },
    { "GT",	    fft_GT },
    { "ST", 	fft_ST },
    { "GE",	    fft_GE },
    { "SE",	    fft_SE },
    { "EQ",	    fft_EQ },
    { "NEQ",	fft_NEQ },
    { "GT0",	fft_GT0 },
    { "ST0",	fft_ST0 },
    { "GE0",	fft_GE0 },
    { "SE0",	fft_SE0 },
    { "EQ0",	fft_EQ0 },
    { "NEQ0",	fft_NEQ0 },
    { "AND",	fft_AND },
    { "OR",	    fft_OR },
    { "NOT",	fft_NOT },
    { "JMP",	fft_JMP },
    { "JGT",	fft_JGT },
    { "JST",	fft_JST },
    { "JGE",	fft_JGE },
    { "JSE",	fft_JSE },
    { "JEQ",	fft_JEQ },
    { "JNEQ",	fft_JNEQ },
    { "JGT0",	fft_JGT0 },
    { "JST0",	fft_JST0 },
    { "JGE0",	fft_JGE0 },
    { "JSE0",	fft_JSE0 },
    { "JEQ0",	fft_JEQ0 },
    { "JNEQ0",	fft_JNEQ0 },
    { "CPY",	fft_CPY },
    { "CGT",	fft_CGT },
    { "CST",	fft_CST },
    { "CGE",	fft_CGE },
    { "CSE",	fft_CSE },
    { "CEQ",	fft_CEQ },
    { "CNEQ",	fft_CNEQ },
    { "CGT0",	fft_CGT0 },
    { "CST0",	fft_CST0 },
    { "CGE0",	fft_CGE0 },
    { "CSE0",	fft_CSE0 },
    { "CEQ0",	fft_CEQ0 },
    { "CNEQ0",	fft_CNEQ0 },
    { "TGT",	fft_TGT },
    { "TST",	fft_TST },
    { "TGE",	fft_TGE },
    { "TSE",	fft_TSE },
    { "TEQ",	fft_TEQ },
    { "TNEQ",	fft_TNEQ },
    { "TGT0",	fft_TGT0 },
    { "TST0",	fft_TST0 },
    { "TGE0",	fft_TGE0 },
    { "TSE0",	fft_TSE0 },
    { "TEQ0",	fft_TEQ0 },
    { "TNEQ0",	fft_TNEQ0 },
    { "UNIT",	fft_UNIT },
    { "UNITT",	fft_UNITT },
    { "URAMP",	fft_URAMP },
    { "TIME",	fft_TIME },
    { "DT",	    fft_DT },
    { "FREQ",	fft_FREQ },
    { "GND",	fft_GND },
    { "RAIL",	fft_RAIL}
};


//***********************************************************************
fileFunctionType identifyFileFunctionType(const char* functionName) {
//***********************************************************************
    if (*functionName == '_')
        functionName++;
    for (const auto& pair : fftNameID)
        if (strcmp(functionName, pair.functionName) == 0)
            return pair.id;
    return fftInvalid;
}


//***********************************************************************
void HMGFileFunction::ReadParams(FunctionDescription& dest, uns nPar, LineTokenizer& lineToken, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    for (uns i = 0; i < nPar; i++) {
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        ParameterIdentifier id;
        if (token[0] == 'R' && token[1] == 'E' && token[2] == 'T') { // ! indexField[0] = ret
            id.parType = ptParam;
            id.parIndex = 0;
        }
        else if (token[0] == 'F' && token[1] == 'R' && token[2] == 'E' && token[3] == 'T') { // ! indexField[0] = ret
            id.parType = ptPrev;
            id.parIndex = 0;
        }
        else {
            if (token[0] == 'P')
                id.parType = ptParam;
            else if (token[0] == 'V')
                id.parType = ptLocalVar;
            else if (token[0] == 'F')
                id.parType = ptPrev;
            else
                throw hmgExcept("HMGFileFunction::ReadParams", "unknown parameter type, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            
            if (sscanf_s(token + 1, "%u", &id.parIndex) != 1)
                throw hmgExcept("HMGFileFunction::ReadParams", "not a number, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            
            if (id.parType == ptParam && id.parIndex >= nParams)
                throw hmgExcept("HMGFileFunction::ReadParams", "parameter index >= number of parameters in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            if (id.parType == ptLocalVar && id.parIndex >= nInternalVars)
                throw hmgExcept("HMGFileFunction::ReadParams", "variable index >= number of variables in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

            // ! indexField[0] = ret, indexField[1] = work field starts, indexField[2...nParam+2-1] = params !

            if (id.parType == ptParam || id.parType == ptPrev)
                id.parIndex += 2;
        }
        dest.parameters.push_back(id);
    }
}


//***********************************************************************
void HMGFileFunction::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read function head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".FUNCTION") != 0)
        throw hmgExcept("HMGFileFunction::Read", ".FUNCTION expected, %s found in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);

    // read model name

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (globalNames.functionNames.contains(token))
        throw hmgExcept("HMGFileFunction::Read", ".FUNCTION %s redefinition in %s, line %u: %s", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    functionIndex = (uns)globalNames.functionNames.size();
    globalNames.functionNames[token] = functionIndex;
    vectorForcedSet(globalNames.functionData, this, functionIndex);

    // read variable and parameter numbers

    if (!lineToken.isSepEOL && !lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileFunction::Read", "P/V=number expected, %s arrived in %s, line %u: %s", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
    while (!lineToken.isSepEOL) {
        if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "P", nParams));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "V", nInternalVars));
        else
            throw hmgExcept("HMGFileFunction::Read", "unknown node/parameter type, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (!lineToken.isSepEOL && !lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
            throw hmgExcept("HMGFileFunction::Read", "simple node name expected, %s arrived (%s) in %s, line %u", lineToken.getActToken(), line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }

    // read function lines

    bool isFunctionNotEnded = true;

    do {
        if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("HMGFileFunction::Read", "incomplete .FUNCTION definition, missing .END in %s", reader.getFileName(lineInfo).c_str());
        
        lineToken.init(line);
        char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        
        if (strcmp(token, ".END") == 0) {
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, "FUNCTION") != 0)
                throw hmgExcept("HMGFileFunction::Read", ".END FUNCTION expected, %s arrived in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (!globalNames.functionNames.contains(token))
                throw hmgExcept("HMGFileFunction::Read", "END .FUNCTION %s: unknown function name in %s, line %u: %s", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            if (globalNames.functionNames[token] != functionIndex)
                throw hmgExcept("HMGFileFunction::Read", ".END FUNCTION %s: wrong function name in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            isFunctionNotEnded = false;
        }
        else {

            // label

            if (labels.contains(token))
                throw hmgExcept("HMGFileFunction::Read", "%s label redefinition in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            labels[token] = (uns)labels.size();
            
            // called function

            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            instructions.emplace_back(FunctionDescription());
            FunctionDescription& func = instructions.back();
            if (token[0] == '_') { // built in function
                func.type = identifyFileFunctionType(token);
                if(func.type == fftInvalid)
                    throw hmgExcept("HMGFileFunction::Read", "unknown built in function: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                switch (func.type) {
                    case fft_CONST:     ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_C_PI:
                    case fft_C_2PI:
                    case fft_C_PI2:
                    case fft_C_E:
                    case fft_C_T0:
                    case fft_C_K:
                    case fft_C_Q:       ReadParams(func, 1, lineToken, reader, line, lineInfo); break;

                    case fft_ADD:
                    case fft_SUB:
                    case fft_MUL:
                    case fft_DIV:       ReadParams(func, 3, lineToken, reader, line, lineInfo); break;

                    case fft_ADDC:
                    case fft_SUBC:
                    case fft_MULC:
                    case fft_DIVC:      ReadParams(func, 2, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_CADD:
                    case fft_CSUB:
                    case fft_CMUL:
                    case fft_CDIV:      ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_NEG:
                    case fft_INV:
                    case fft_SQRT:      ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_POW:       ReadParams(func, 3, lineToken, reader, line, lineInfo); break;
                    case fft_POWC:      ReadParams(func, 2, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_CPOW:      ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_EXP:
                    case fft_NEXP:
                    case fft_IEXP:
                    case fft_INEXP:
                    case fft_LN:        ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_LOG:       ReadParams(func, 3, lineToken, reader, line, lineInfo); break;
                    case fft_CLOG:      ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        ReadParams(func, 1, lineToken, reader, line, lineInfo);
                                        break;

                    case fft_ABS:
                    case fft_ASIN:
                    case fft_ACOS:
                    case fft_ATAN:
                    case fft_ASINH:
                    case fft_ACOSH:
                    case fft_ATANH:
                    case fft_SIN:
                    case fft_COS:
                    case fft_TAN:
                    case fft_SINH:
                    case fft_COSH:
                    case fft_TANH:      ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_RATIO:     ReadParams(func, 4, lineToken, reader, line, lineInfo); break;

                    case fft_PWL: {
                            ReadParams(func, 1, lineToken, reader, line, lineInfo); break;
                            rvt x, y;
                            while (!lineToken.isSepEOL) {
                                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                                if (!spiceTextToRvt(token, x))
                                    throw hmgExcept("HMGFileFunction::Read", "x value not a number: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                                if(lineToken.isSepEOL)
                                    throw hmgExcept("HMGFileFunction::Read", "unexpected end of line: y value is missing in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                                if (!spiceTextToRvt(token, y))
                                    throw hmgExcept("HMGFileFunction::Read", "y value not a number: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                                func.values.push_back(x);
                                func.values.push_back(y);
                            }
                        }
                        break;
                    case fft_DERIV:     ReadParams(func, 4, lineToken, reader, line, lineInfo); break;
                    case fft_DERIVC:    ReadParams(func, 3, lineToken, reader, line, lineInfo);
                                        ReadValue(func, lineToken, reader, line, lineInfo);
                                        break;
                    case fft_VLENGTH2:  ReadParams(func, 3, lineToken, reader, line, lineInfo); break;
                    case fft_VLENGTH3:  ReadParams(func, 4, lineToken, reader, line, lineInfo); break;
                    case fft_DISTANCE2: ReadParams(func, 5, lineToken, reader, line, lineInfo); break;
                    case fft_DISTANCE3: ReadParams(func, 7, lineToken, reader, line, lineInfo); break;

                    case fft_GT:
                    case fft_ST:
                    case fft_GE:
                    case fft_SE:
                    case fft_EQ:
                    case fft_NEQ:       ReadParams(func, 3, lineToken, reader, line, lineInfo); break;

                    case fft_GT0:
                    case fft_ST0:
                    case fft_GE0:
                    case fft_SE0:
                    case fft_EQ0:
                    case fft_NEQ0:      ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_AND:
                    case fft_OR:        ReadParams(func, 3, lineToken, reader, line, lineInfo); break;

                    case fft_NOT:       ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_JMP:       ReadLabel(func, lineToken, reader, lineInfo); break;
                    case fft_JGT:
                    case fft_JST:
                    case fft_JGE:
                    case fft_JSE:
                    case fft_JEQ:
                    case fft_JNEQ:      ReadLabel(func, lineToken, reader, lineInfo);
                                        ReadParams(func, 2, lineToken, reader, line, lineInfo); 
                                        break;

                    case fft_JGT0:
                    case fft_JST0:
                    case fft_JGE0:
                    case fft_JSE0:
                    case fft_JEQ0:
                    case fft_JNEQ0:     ReadLabel(func, lineToken, reader, lineInfo);
                                        ReadParams(func, 1, lineToken, reader, line, lineInfo); 
                                        break;

                    case fft_CPY:       ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_CGT:
                    case fft_CST:
                    case fft_CGE:
                    case fft_CSE:
                    case fft_CEQ:
                    case fft_CNEQ:      ReadParams(func, 4, lineToken, reader, line, lineInfo); break;

                    case fft_CGT0:
                    case fft_CST0:
                    case fft_CGE0:
                    case fft_CSE0:
                    case fft_CEQ0:
                    case fft_CNEQ0:     ReadParams(func, 3, lineToken, reader, line, lineInfo); break;

                    case fft_TGT:
                    case fft_TST:
                    case fft_TGE:
                    case fft_TSE:
                    case fft_TEQ:
                    case fft_TNEQ:      ReadParams(func, 5, lineToken, reader, line, lineInfo); break;

                    case fft_TGT0:
                    case fft_TST0:
                    case fft_TGE0:
                    case fft_TSE0:
                    case fft_TEQ0:
                    case fft_TNEQ0:     ReadParams(func, 4, lineToken, reader, line, lineInfo); break;

                    case fft_UNIT:      ReadParams(func, 2, lineToken, reader, line, lineInfo); break;
                    case fft_UNITT:     ReadParams(func, 1, lineToken, reader, line, lineInfo); break;
                    case fft_URAMP:     ReadParams(func, 2, lineToken, reader, line, lineInfo); break;

                    case fft_TIME:
                    case fft_DT:
                    case fft_FREQ:
                    case fft_GND:       ReadParams(func, 1, lineToken, reader, line, lineInfo); break;
                    case fft_RAIL:      ReadParams(func, 2, lineToken, reader, line, lineInfo); break;
                    default:
                        throw hmgExcept("HMGFileFunction::Read", "unknown built in function type ID (%u) in %s, line %u: %s", func.type, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                }
            }
            else { // custom function
                
                if (!globalNames.functionNames.contains(token))
                    throw hmgExcept("HMGFileFunction::Read", "unknown function name %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                
                func.type = fftCustom;
                func.customIndex = globalNames.functionNames[token];

                cuns nPar = globalNames.functionData[func.customIndex]->nParams;

                ReadParams(func, nPar, lineToken, reader, line, lineInfo);
            }
        }
    } while (isFunctionNotEnded);

    // jump labels to ID

    for (uns i = 0; i < instructions.size(); i++) {
        FunctionDescription& func = instructions[i];
        if (func.labelID == unsMax) {
            if (!labels.contains(func.labelName))
                throw hmgExcept("HMGFileFunction::Read", "%s jump label missing in %s function", func.labelName.c_str(), token); // token contains the function name
            func.labelID = labels[func.labelName];
        }
    }
}


//***********************************************************************
void HMGFileGlobalDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    bool isStop = false;
    LineTokenizer lineToken;

    while(!isStop && reader.getLine(line, MAX_LINE_LENGHT, lineInfo)) {

        if (line[0] == '.') {
            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, ".MODEL") == 0) {
                HMGFileModelDescription* pModel = new HMGFileModelDescription;
                itemList.push_back(pModel);
                pModel->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".PROBE") == 0) {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (globalNames.probeNames.contains(token)) {
                    globalNames.probeData[globalNames.probeNames[token]]->Read(reader, line, lineInfo, true);
                }
                else {
                    HMGFileProbe* pCreate = new HMGFileProbe;
                    itemList.push_back(pCreate);
                    pCreate->Read(reader, line, lineInfo, false);
                }
            }
            else if (strcmp(token, ".SUNREDTREE") == 0) {
                HMGFileSunredTree* pSunredTree = new HMGFileSunredTree;
                itemList.push_back(pSunredTree);
                pSunredTree->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".MULTIGRID") == 0) {
                HMGFileMultiGrid* pMultiGrid = new HMGFileMultiGrid;
                itemList.push_back(pMultiGrid);
                pMultiGrid->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".RAILS") == 0) {
                HMGFileRails* pRails = new HMGFileRails;
                itemList.push_back(pRails);
                pRails->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".FUNCTION") == 0) {
                HMGFileFunction* pFunction = new HMGFileFunction;
                itemList.push_back(pFunction);
                pFunction->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".CREATE") == 0) {
                HMGFileCreate* pCreate = new HMGFileCreate;
                itemList.push_back(pCreate);
                pCreate->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".RUN") == 0) {
                HMGFileRun* pRun = new HMGFileRun;
                itemList.push_back(pRun);
                pRun->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".SAVE") == 0) {
                HMGFileSave* pSave = new HMGFileSave;
                itemList.push_back(pSave);
                pSave->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".SET") == 0) {
                HMGFileSet* pSet = new HMGFileSet;
                itemList.push_back(pSet);
                pSet->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".END") == 0) {
                isStop = true;
            }
/*///            else if (strcmp(token, ".REPLACE") == 0) {
                // az eredeti komponens NameToIndex-eit haszn�lja, �gy ak�rh�ny replace ut�n is ugyanarra a n�vre ugyanazt az indexet adja, 
                // de am�gy �resen indul
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                while (*token == '@')token++;
                while (*token == ':')token++;
                // subcktNames, controllerNames, componentTemplateNames, modelNames
                unsigned index;
                if ((index = globalNames.subcktNames.find(token)) != 0) {
                    SpiceSubcktDescription* psubckt = new SpiceSubcktDescription;
                    itemList.push_back(psubckt);
                    psubckt->Replace(globalNames.subcktData[index], reader, line, lineInfo);
                    //vectorForcedSet(globalNames.subcktData, psubckt, psubckt->subcktIndex); // a t�nyleges csere majd a szimul�ci�n�l lesz
                }
                else if ((index = globalNames.controllerNames.find(token)) != 0) {
                    SpiceControllerDescription* pController = new SpiceControllerDescription;
                    itemList.push_back(pController);
                    pController->Replace(globalNames.controllerData[index], reader, line, lineInfo);
                    //vectorForcedSet(globalNames.controllerData, pController, pController->controllerIndex);
                }
                else if ((index = globalNames.componentTemplateNames.find(token)) != 0) {
                    SpiceComponentTemplateLine* pCompTemp = new SpiceComponentTemplateLine;
                    itemList.push_back(pCompTemp);
                    pCompTemp->Replace(globalNames.componentTemplateData[index], reader, line, lineInfo, fullName);
                    //vectorForcedSet(globalNames.componentTemplateData, pCompTemp, pCompTemp->componentTemplateIndex);
                }
                else if ((index = globalNames.modelNames.find(token)) != 0) {
                    TODO("HMGFileGlobalDescription::Read: .REPLACE .MODEL");
                }
                else
                    throw hmgExcept("HMGFileGlobalDescription::Read", "unrecognised .REPLACE name (%s) arrived in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            }
*///            
            else
                throw hmgExcept("HMGFileGlobalDescription::Read", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else
            throw("HMGFileGlobalDescription::Read", "this line type is not supported yet in Hex Open: %s", line);
    }
}


}
