//***********************************************************************
// HexMG HMG File Converter Source
// Creation date:  2021. 06. 20.
// Creator:        László Pohl
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
    enum States { stStart, stEOF, stEOL, stCommentLine };
    States st = stStart;
    while (isNotFinished) {
        int ch = isPeek ? peekChar() : getChar();
        if (ch == EOF)
            st = stEOF;
        else if (ch == '\n')
            st = stEOL;
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
bool GlobalHMGFileNames::textToProbeNodeID(char* token, uns fullCircuitIndex, ProbeNodeID& dest) {
//***********************************************************************
    uns componentIndex = 0;
    HMGFileModelDescription* currentComponent = modelData[fullCircuitData[fullCircuitIndex]->modelID];
    while (true) {
        uns i = 0;
        while (token[i] != '\0' && token[i] != '.')
            i++;
        if (token[i] == '.') {
            token[i] = '\0';
            if (currentComponent == nullptr || componentIndex >= probeMaxComponentLevel)
                return false;
            uns ci = dest.componentID[componentIndex] = currentComponent->instanceListIndex[token];
            HMGFileComponentInstanceLine* pxline = currentComponent->instanceList[ci];
            currentComponent = pxline->instanceOfWhat == itModel ? modelData[pxline->indexOfTypeInGlobalContainer] : nullptr;
            token[i] = '.';
            componentIndex++;
            token += i + 1;
        }
        else {
            if (!textToSimpleNodeID(token, dest.nodeID))
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
        throw hmgExcept("SpiceSubcktDescription::Read", "The .hmg file must start with HEXMG, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    uns mainVersion, subVersion;
    if (sscanf(token, "%u.%u", &mainVersion, &subVersion) != 2)
        throw hmgExcept("SpiceSubcktDescription::Read", "The first line must be HEXMG mainversion.subversion, HEXMG %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    if (mainVersion > 1 || subVersion > 0)
        throw hmgExcept("SpiceSubcktDescription::Read", "Supported .hmg version is <= 1.0, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // reading the simulation

    globalCircuit.Read(reader, line, lineInfo);
    most("Read");
    globalCircuit.ProcessXLines();
    most("ProcessXLines");
    globalCircuit.ProcessExpressionNames(&globalCircuit.itemList);
    most("ProcessExpressionNames");
}


//***********************************************************************
void HMGFileModelDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read subcircuit head (if not the global circuit is readed)

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

    nIONodes = nNormalINodes = nControlNodes = nNormalONodes = nForwardedONodes = 0;
    nNormalInternalNodes = nControlInternalNodes = nInternalVars = nParams = 0;

    if (!lineToken.isSepEOL && !lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("HMGFileModelDescription::Read", "node/parameter=number expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    while (!lineToken.isSepEOL) {
        if      (readNodeOrParNumber(line, lineToken, reader, lineInfo, "X", nIONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "N", nNormalInternalNodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "P", nParams));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "IN", nNormalINodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "CIN", nControlNodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "OUT", nNormalONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "FWOUT", nForwardedONodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "C", nControlInternalNodes));
        else if (readNodeOrParNumber(line, lineToken, reader, lineInfo, "VAR", nInternalVars));
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

    sumExternalNodes = nIONodes + nNormalINodes + nControlNodes + nNormalONodes + nForwardedONodes;
    sumInternalNodes = nNormalInternalNodes + nControlInternalNodes;
        
    if (modelType == hfmtSubcircuit)
        ReadOrReplaceBodySubcircuit(reader, line, lineInfo, false);
    else if (modelType == hfmtController)
        ReadOrReplaceBodyController(reader, line, lineInfo, false);
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
        ReadOrReplaceBodySubcircuit(reader, line, lineInfo, false);
    else if (modelType == hfmtController)
        ReadOrReplaceBodyController(reader, line, lineInfo, false);
    else
        throw hmgExcept("HMGFileModelDescription::Read", "Internal error: unknown model type (%u)", modelType);
}


//***********************************************************************
void HMGFileModelDescription::ReadOrReplaceBodySubcircuit(ReadALine& reader, char* line, LineInfo& lineInfo, bool) {
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
                SimpleNodeID rail;
                if (!textToSimpleNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                SimpleNodeID node;
                while (!lineToken.isSepEOL) {
                    lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    if (!textToSimpleNodeID(lineToken.getActToken(), node))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (!checkNodeValidity(node))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAIL: invalid node index: %u in %s, line %u: %s", node.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    if (defaults.size() > 0 && std::get<0>(defaults.back()) == rail.index && std::get<1>(defaults.back()) == node.type) {
                        auto& last = defaults.back();
                        if (node.index == std::get<2>(last) - 1)
                            std::get<2>(last)--;
                        else if (node.index == std::get<3>(last) + 1)
                            std::get<3>(last)++;
                        else
                            defaults.push_back(std::make_tuple(rail.index, node.type, node.index, node.index));
                    }
                    else
                        defaults.push_back(std::make_tuple(rail.index, node.type, node.index, node.index));
                }
            }
            else if (strcmp(token, ".DEFAULTRAILRANGE") == 0) {
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                SimpleNodeID rail;
                if (!textToSimpleNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                SimpleNodeID node1, node2;
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!textToSimpleNodeID(lineToken.getActToken(), node1))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                if (!textToSimpleNodeID(lineToken.getActToken(), node2))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: wrong node ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if (!checkNodeValidity(node1))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: invalid node index: %u in %s, line %u: %s", node1.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if (!checkNodeValidity(node2))
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: invalid node index: %u in %s, line %u: %s", node2.index, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                if(node1.type != node2.type)
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", ".DEFAULTRAILRANGE: node range required in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                defaults.push_back(std::make_tuple(rail.index, node1.type, node1.index < node2.index ? node1.index : node2.index, node1.index < node2.index ? node2.index : node1.index));
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
            bool isController = false;

            // component type

            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

            pxline->instanceOfWhat = itNone;
            pxline->indexOfTypeInGlobalContainer = bimtSize;

            uns nodenum = 2, parnum = 1, funcnum = 0;
            if (strcmp(token, "MODEL") == 0) {
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                pxline->instanceOfWhat = itModel;
                try { pxline->indexOfTypeInGlobalContainer = globalNames.modelNames.at(token); }
                catch (const std::out_of_range&) {
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised MODEL (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                }
                const HMGFileModelDescription& mod = *globalNames.modelData[pxline->indexOfTypeInGlobalContainer];
                nodenum = mod.sumExternalNodes;
                parnum = mod.nParams;
                isController = mod.modelType == hfmtController;
            }
            else if (strcmp(token,  "R") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstR_1; }
            else if (strcmp(token, "R2") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstR_2; parnum = 2; }
            else if (strcmp(token,  "G") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstG_1; }
            else if (strcmp(token, "G2") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstG_2; parnum = 2; }
            else if (strcmp(token,  "C") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstC_1; }
            else if (strcmp(token, "C2") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstC_2; parnum = 2; }
            else if (strcmp(token,  "I") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstI_1; parnum = 4; }
            else if (strcmp(token, "I2") == 0) { pxline->instanceOfWhat = itBuiltInComponentType; pxline->indexOfTypeInGlobalContainer = bimtConstI_2; parnum = 5; }

            // read nodes and parameters
            
            if (pxline->instanceOfWhat != itNone) {
                for (uns i = 0; i < nodenum; i++) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    pxline->nodes.emplace_back(SimpleNodeID());
                    if (!textToSimpleNodeID(token, pxline->nodes.back()))
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
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
                    if (pxline->indexOfTypeInGlobalContainer == bimtConstI_1 || pxline->indexOfTypeInGlobalContainer == bimtConstI_2) {
                        uns index = parnum;
                        if (     strcmp(token, "DC0") == 0) index = 0;
                        else if (strcmp(token, "DC")  == 0) index = 1;
                        else if (strcmp(token, "AC")  == 0) index = 2;
                        else if (strcmp(token, "PHI") == 0) index = 3;
                        else if (strcmp(token, "MUL") == 0) index = 4; // for bimtConstI_1 index = 4 == parnum => the error is handled in the next if-else section => it cannot be in an else branch
                        if (index < parnum) {
                            token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                            if (isalpha(token[0])) { // parameter / variable / node
                                if (!textToSimpleNodeID(token, pxline->params[index].param))
                                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised parameter/variable/node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                            }
                            else { // value
                                double val = 0;
                                if (sscanf_s(token, "%lg", &val) != 1)
                                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                                pxline->params[index].value = (rvt)val;
                            }
                            continue;
                        }
                    }
                    // this part cannot be put in else section ! (see above):
                    if (isalpha(token[0])) { // parameter / variable / node
                        if (!textToSimpleNodeID(token, pxline->params[i].param))
                            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised parameter/variable/node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    }
                    else { // value
                        double val = 0;
                        if (sscanf_s(token, "%lg", &val) != 1)
                            throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised value (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                        pxline->params[i].value = (rvt)val;
                    }
                }
                for (uns i = 0; i < funcnum; i++) {
                    TODO("Read function calls");
                }
            }

            if (!lineToken.isSepEOL) {
                bool isDefRail = (strcmp(token, "DEFAULTRAIL") == 0);
                if (!isDefRail) { // for e.g. current source the parameters can be less than the formal number of paramteres
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    isDefRail = (strcmp(token, "DEFAULTRAIL") == 0);
                }
                if (isDefRail) {
                    token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                    SimpleNodeID rail;
                    if (!textToSimpleNodeID(lineToken.getActToken(), rail) || rail.type != nvtRail)
                        throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "DEFAULTRAIL: missing rail ID in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                    pxline->isDefaultRail = true;
                    pxline->defaultRailIndex = rail.index;
                }
                else
                    throw hmgExcept("HMGFileModelDescription::ReadOrReplaceBody", "unrecognised line ending (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
            }

            // controller instance or node instance => setting instance index

            if (isController) {
                if (controllerInstanceNameIndex.contains(instanceName))
                    throw hmgExcept("HMGFileModelDescription::Read", "%s redefinition in %s, line %u: %s", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                pxline->instanceIndex = (uns)controllerInstanceNameIndex.size();
                controllerInstanceNameIndex[instanceName] = pxline->instanceIndex;
            }
            else {
                if (componentInstanceNameIndex.contains(instanceName))
                    throw hmgExcept("HMGFileModelDescription::Read", "%s redefinition in %s, line %u: %s", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
                pxline->instanceIndex = (uns)componentInstanceNameIndex.size();
                componentInstanceNameIndex[instanceName] = pxline->instanceIndex;
            }
            instanceListIndex[instanceName] = (uns)instanceListIndex.size();
        }
    } while (isModelNotEnded);
}


//***********************************************************************
void HMGFileModelDescription::ProcessXLines() {
//***********************************************************************
/*
    LineTokenizer lineToken;
    const char* token = nullptr;

    for (HMGFileListItem* pItem : itemList) {
        if (pItem->getItemType() == itModel)
            static_cast<HMGFileModelDescription*>(pItem)->ProcessXLines();
        else if (pItem->getItemType() == itComponentInstance) { // normal nodes
            HMGFileComponentInstanceLine* pInstance = static_cast<HMGFileComponentInstanceLine*>(pItem);
            switch (pInstance->theLine[0]) {
                case 'R': 
                case 'C':
                case 'V':
                case 'I':
                {
                    pInstance->instanceOfWhat = itBuiltInComponentType;
                    if (pInstance->theLine[0] == 'R') pInstance->indexOfTypeInGlobalContainer = bictt_R;
                    else if (pInstance->theLine[0] == 'C') pInstance->indexOfTypeInGlobalContainer = bictt_C;
                    else if (pInstance->theLine[0] == 'V') pInstance->indexOfTypeInGlobalContainer = bictt_V;
                    else if (pInstance->theLine[0] == 'I') pInstance->indexOfTypeInGlobalContainer = bictt_I;
                    else throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "imposs");

                    lineToken.init(pInstance->theLine.c_str());
                    lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // name ignored

                    pInstance->nodes.resize(2);
                    unsigned parentVectorIndex = 0;
                    for (unsigned i = 0; i < 2; i++, parentVectorIndex++) {
                        token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                        while (*token == '@')token++;
                        if (strchr(token, '.') != nullptr)
                            throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                "only local external / internal node is accepted as normal node, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                        if ((token[0] == '0' && token[1] == 0) || (token[0] == ':' && token[1] == '0' && token[2] == 0)) {
                            pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntGround;
                            pInstance->nodes[parentVectorIndex].componentId = 0;
                            pInstance->nodes[parentVectorIndex].nodeId = 0;
                        }
                        else if (token[0] == '$' || (token[0] == ':' && token[1] == '$')) {
                            const bool isColon = token[0] == ':';
                            char num = isColon ? token[2] : token[1];
                            if (num < '0' || num > '9')
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "special ground node number missing, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                            if(token[isColon ? 3 : 2] != 0)
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "special ground node must be $0 ... $9, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                            pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntGround;
                            pInstance->nodes[parentVectorIndex].componentId = 0;
                            pInstance->nodes[parentVectorIndex].nodeId = num - '0';
                        }
                        else {
                            //unsigned nodeIndex = isReplacer ? pParent->externalNodeNames.find(token) : externalNodeNames.find(token);
                            //if (nodeIndex > 0) {
                            //    pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntExternal;
                            //    pInstance->nodes[parentVectorIndex].componentId = 0;
                            //    pInstance->nodes[parentVectorIndex].nodeId = nodeIndex;
                            //}
                            //else {
                            //    nodeIndex = isReplacer ? pParent->internalNodeNames.findOrAdd(token) : internalNodeNames.findOrAdd(token);
                            //    pInstance->nodes[parentVectorIndex].nodeType = token[0] == 'T' ? NodeID::ntThermal : 0;
                            //    pInstance->nodes[parentVectorIndex].componentId = 0;
                            //    pInstance->nodes[parentVectorIndex].nodeId = nodeIndex;
                            //}
                        }
                        if (parentVectorIndex < pInstance->nodes.size() - 1) {
                            if (lineToken.isSepEOL || lineToken.isSepComma || lineToken.isSepOpeningBracket)
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "missing node definition in %s, line %u: %s)", pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                        }
                    }

                    // TODO: check model names to identify semiconductor resistor/capacitor
                    // TODO: IC= for capacitor

                    if (pInstance->indexOfTypeInGlobalContainer == bictt_V || pInstance->indexOfTypeInGlobalContainer == bictt_I) {
                        lineToken.storePosition();
                        token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                        if (strcmp(token, "DC") != 0) // TODO AC, TRAN, etc
                            lineToken.loadPosition(); // format: "Vxx N1 N2 DC value" or "Vxx N1 N2 value" => ignoring "DC"
                    }

                    token = lineToken.getRestOfTheLine();
                    if (!pInstance->valueExpression.buildFromString(token))
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                            "invalid expression: %s in %s, line %u: %s)", pInstance->valueExpression.errorMessage.c_str(), pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());

                }
                break;
                case 'X': {
                    lineToken.init(pInstance->theLine.c_str());
                    lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // name ignored
                    if(lineToken.isSepEOL || lineToken.isSepOpeningBracket)
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                            "invalid component instance line in %s, line %u: %s)", pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());

                    unsigned nNodes;
                    for (nNodes = 0; !lineToken.isSepEOL && !lineToken.isSepOpeningBracket; nNodes++) // counting nodes and finding type
                        token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                    nNodes--;

                    if ((pInstance->indexOfTypeInGlobalContainer = globalNames.componentTemplateNames.findMultiLevel(fullName.length() > 0 ? fullName + "." + token : token)) != 0) {
                        pInstance->instanceOfWhat = itComponentTemplate;
                        pInstance->nNormalNode  = globalNames.componentTemplateData[pInstance->indexOfTypeInGlobalContainer]->nNormalNode;
                        pInstance->nControlNode = globalNames.componentTemplateData[pInstance->indexOfTypeInGlobalContainer]->nControlNode;
                        pInstance->nParams      = globalNames.componentTemplateData[pInstance->indexOfTypeInGlobalContainer]->nParams;
                    }
                    else if ((pInstance->indexOfTypeInGlobalContainer = globalNames.controllerNames.findMultiLevel(fullName.length() > 0 ? fullName + "." + token : token)) != 0) {
                        pInstance->instanceOfWhat = itController;
                        pInstance->nNormalNode  = 0;
                        pInstance->nControlNode = globalNames.controllerData[pInstance->indexOfTypeInGlobalContainer]->nControlNode;
                        pInstance->nParams      = globalNames.controllerData[pInstance->indexOfTypeInGlobalContainer]->nParams;
                    }
                    else if ((pInstance->indexOfTypeInGlobalContainer = globalNames.subcktNames.findMultiLevel(fullName.length() > 0 ? fullName + "." + token : token)) != 0) {
                        pInstance->instanceOfWhat = itSubckt;
                        pInstance->nNormalNode  = globalNames.subcktData[pInstance->indexOfTypeInGlobalContainer]->nNormalNode;
                        pInstance->nControlNode = globalNames.subcktData[pInstance->indexOfTypeInGlobalContainer]->nControlNode;
                        pInstance->nParams      = globalNames.subcktData[pInstance->indexOfTypeInGlobalContainer]->nParams;
                    }
                    else
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "unknown type (%s) in %s, line %u: %s)",
                            token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                    
                    if(nNodes != unsigned(pInstance->nNormalNode) + unsigned(pInstance->nControlNode))
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "nNodes != pInstance->nNormalNode + pInstance->nControlNode (%u != %u + %u) in %s, line %u: %s)", 
                            nNodes, pInstance->nNormalNode, pInstance->nControlNode, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());

                    lineToken.init(pInstance->theLine.c_str());
                    lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // name ignored

                    pInstance->nodes.resize(nNodes);
                    unsigned parentVectorIndex = 0;
                    for (unsigned i = 0; i < pInstance->nNormalNode; i++, parentVectorIndex++) {
                        token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                        while (*token == '@')token++;
                        if (strchr(token, '.') != nullptr)
                            throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                "only local external / internal node is accepted as normal node, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                        if ((token[0] == '0' && token[1] == 0) || (token[0] == ':' && token[1] == '0' && token[2] == 0)) {
                            pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntGround;
                            pInstance->nodes[parentVectorIndex].componentId = 0;
                            pInstance->nodes[parentVectorIndex].nodeId = 0;
                        }
                        else if (token[0] == '$' || (token[0] == ':' && token[1] == '$')) {
                            const bool isColon = token[0] == ':';
                            char num = isColon ? token[2] : token[1];
                            if (num < '0' || num > '9')
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "special ground node number missing, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                            if (token[isColon ? 3 : 2] != 0)
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "special ground node must be $0 ... $9, %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                            pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntGround;
                            pInstance->nodes[parentVectorIndex].componentId = 0;
                            pInstance->nodes[parentVectorIndex].nodeId = num - '0';
                        }
                        else {
                            //unsigned nodeIndex = isReplacer ? pParent->externalNodeNames.find(token) : externalNodeNames.find(token);
                            //if (nodeIndex > 0) {
                            //    pInstance->nodes[parentVectorIndex].nodeType = NodeID::ntExternal;
                            //    pInstance->nodes[parentVectorIndex].componentId = 0;
                            //    pInstance->nodes[parentVectorIndex].nodeId = nodeIndex;
                            //}
                            //else {
                            //    nodeIndex = isReplacer ? pParent->internalNodeNames.findOrAdd(token) : internalNodeNames.findOrAdd(token);
                            //    pInstance->nodes[parentVectorIndex].nodeType = token[0] == 'T' ? NodeID::ntThermal : 0;
                            //    pInstance->nodes[parentVectorIndex].componentId = 0;
                            //    pInstance->nodes[parentVectorIndex].nodeId = nodeIndex;
                            //}
                        }
                        if (parentVectorIndex < pInstance->nodes.size() - 1) {
                            if (lineToken.isSepEOL || lineToken.isSepComma || lineToken.isSepOpeningBracket)
                                throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                                    "missing node definition in %s, line %u: %s)", pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                        }
                    }
                }
                break;
            }
        }
    }
    for (HMGFileListItem* pItem : itemList) {
        if (pItem->getItemType() == itComponentInstance) { // control nodes
            HMGFileComponentInstanceLine* pInstance = static_cast<HMGFileComponentInstanceLine*>(pItem);

            lineToken.init(pInstance->theLine.c_str());

            lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // name ignored

            unsigned parentVectorIndex = 0;
            for (unsigned i = 0; i < pInstance->nNormalNode; i++, parentVectorIndex++) {
                lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // normal nodes ignored
            }

            for (unsigned i = 0; i < pInstance->nControlNode; i++, parentVectorIndex++) {
                token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                if (!identifyNode(pInstance->nodes[parentVectorIndex], token)) {
                    if (strchr(token, '.') != nullptr)
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                            "component internal node name cannot contain a ., %s arrived in %s, line %u", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
                    unsigned nodeIndex;
                    if (!(isReplacer ? pParent->internalNodeNames.add(token, nodeIndex) : internalNodeNames.add(token, nodeIndex)))
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "program error: existing or not existing node?: %s)", token);
                    pInstance->nodes[parentVectorIndex].nodeType = token[0] == 'T' ? NodeID::ntThermal : 0;
                    pInstance->nodes[parentVectorIndex].componentId = 0;
                    pInstance->nodes[parentVectorIndex].nodeId = nodeIndex;
                }

                if (parentVectorIndex < pInstance->nodes.size() - 1) {
                    if (lineToken.isSepEOL || lineToken.isSepComma || lineToken.isSepOpeningBracket)
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines",
                            "missing node definition (%s) in %s, line %u: %s)", token, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                }
            }

            lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // type ignored

            // params

            if (pInstance->nParams > 0) {
                if (!lineToken.isSepOpeningBracket)
                    throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "parent parameter list is missing in %s, line %u: %s", 
                        pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                pInstance->params.resize(pInstance->nParams);
                for (unsigned i = 0; i < pInstance->nParams; i++) {
                    lineToken.skipSeparators();
                    LineTokenizer::ExpressionToken xtok = lineToken.getNextExpressionToken();
                    if (xtok.exprType == LineTokenizer::ExpressionToken::etNumber) {
                        pInstance->params[i].paramIndex = 0;
                        pInstance->params[i].value = xtok.value;
                    }
                    else if (xtok.exprType == LineTokenizer::ExpressionToken::etName) {
                        //pInstance->params[i].paramIndex = isReplacer ? pParent->constParameterNames.find(xtok.name) : constParameterNames.find(xtok.name);
                        //if (pInstance->params[i].paramIndex == 0)
                        //    throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "unknown parent parameter name (%s) in %s, line %u: %s", 
                        //        xtok.name.c_str(), pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                        //pInstance->params[i].value = 0;
                    }
                    else
                        throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "missing or incorrect parent parameter %u in %s, line %u: %s", 
                            i + 1, pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
                }
                if (!lineToken.isSepClosingBracket)
                    throw hmgExcept("SpiceSubcktDescription::ProcessXLines", "parent parameter list closing ) is missing in %s, line %u: %s", 
                        pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, lineToken.getLine());
            }
        }
    }
*/
}


//***********************************************************************
void HMGFileModelDescription::ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList) {
//***********************************************************************
//    for (HMGFileListItem* pItem : (pItemList== nullptr ? itemList : *pItemList)) {
//        switch (pItem->getItemType()) {
//            case itNone: break;
//            case itComponentInstance: {
//                HMGFileComponentInstanceLine* pInstance = static_cast<HMGFileComponentInstanceLine*>(pItem);
///                    for (auto& it : pInstance->valueExpression.theExpression) {
///                        if (!identifyExpressionNodeOrPar(it))
///                            throw hmgExcept("SpiceComponentTemplateLine::ProcessExpressionNames", "unknown identifier (%s) in %s, line %u: %s", 
///                                it.name.c_str(), pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, pInstance->theLine.c_str());
///                    }
//                }
//                break;
//            case itModel: static_cast<HMGFileModelDescription*>(pItem)->ProcessExpressionNames(nullptr); break;
///            case itComponentTemplate: static_cast<SpiceComponentTemplateLine*>(pItem)->ProcessExpressionNames(); break;
//            case itBuiltInComponentType: break;
///            case itController: static_cast<SpiceControllerDescription*>(pItem)->ProcessExpressionNames(nullptr); break;
///            case itExpression: {
///                    SpiceExpressionLine* pExpression = static_cast<SpiceExpressionLine*>(pItem);
///                    for (auto& it : pExpression->theExpression.theExpression) {
///                        if (!identifyExpressionNodeOrPar(it))
///                            throw hmgExcept("SpiceComponentTemplateLine::ProcessExpressionNames", "unknown identifier (%s) in %s", it.name.c_str(), pExpression->fullName.c_str());
///                    }
///                }
///                break;
//        }
//    }
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
        SimpleNodeID nid;
        if (!textToSimpleNodeID(token, nid))
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
    SimpleNodeID nid;
    if (!textToSimpleNodeID(token, nid))
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

        if (     strcmp(token, "V"      ) == 0) probeType = ptV;
        else if (strcmp(token, "I"      ) == 0) probeType = ptI;
        else if (strcmp(token, "SUM"    ) == 0) probeType = ptSum;
        else if (strcmp(token, "AVERAGE") == 0) probeType = ptAverage;
        else
            throw hmgExcept("HMGFileCreate::Read", "Unknown probe type (%s) in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

        // fullCircuit

        token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        try { fullCircuitID = globalNames.fullCircuitNames.at(token); }
        catch (const std::out_of_range&) {
            throw hmgExcept("HMGFileCreate::Read", "unrecognised MODEL (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
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
        else if (strcmp(token, "SUM") == 0) {
            if (probeType != ptSum)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else if (strcmp(token, "AVERAGE") == 0) {
            if (probeType != ptAverage)
                throw hmgExcept("HMGFileCreate::Read", "A different node type than previously specified: %s in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }

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
        ProbeNodeID pnid;
        if(!globalNames.textToProbeNodeID(token, fullCircuitID, pnid))
            throw hmgExcept("HMGFileCreate::Read", "unrecognised node (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        nodes.push_back(pnid);
    }
}


//***********************************************************************
void HMGFileSunredTree::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    LineTokenizer lineToken;

    // read subcircuit head (if not the global circuit is readed)

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
    vectorForcedSet(globalNames.sunredTreeData, this, sunredTreeIndex);

    ReadOrReplaceBody(reader, line, lineInfo, false);
}


//***********************************************************************
void HMGFileSunredTree::Replace(HMGFileSunredTree* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    sunredTreeIndex = pParent->sunredTreeIndex;
    fullName = parent->fullName;
    ReadOrReplaceBody(reader, line, lineInfo, true);
}


//***********************************************************************
void HMGFileSunredTree::ReadOrReplaceBody(ReadALine& reader, char* line, LineInfo& lineInfo, bool) {
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

                // store

                reductions.push_back({ src1, src2, dest });
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
                isSunredTreeNotEnded = false;
            }
            else
                throw hmgExcept("HMGFileSunredTree::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
    } while (isSunredTreeNotEnded);
}


//***********************************************************************
void HMGFileGlobalDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    bool isStop = false;
    LineTokenizer lineToken;

    while(!isStop && reader.getLine(line, MAX_LINE_LENGHT, lineInfo)) {

        //if (line[0] == 'X' || line[0] == 'R' || line[0] == 'C' || line[0] == 'V' || line[0] == 'I') {
        //    SpiceComponentInstanceLine* pxline = new SpiceComponentInstanceLine;
        //    itemList.push_back(pxline);
        //
        //    pxline->theLine = line;
        //    pxline->theLineInfo = lineInfo;
        //    pxline->fileName = reader.getFileName(lineInfo);
        //
        //    lineToken.init(line);
        //    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        //    if (!componentInstanceNames.add(token, pxline->componentInstanceIndex))
        //        throw hmgExcept("HMGFileGlobalDescription::Read", "%s redefinition in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        //    vectorForcedSet< HMGFileListItem* >(componentInstances, pxline, pxline->componentInstanceIndex);
        //}

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
                HMGFileSunredTree* psunredtree = new HMGFileSunredTree;
                itemList.push_back(psunredtree);
                psunredtree->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".RAILS") == 0) {
                HMGFileRails* pRails = new HMGFileRails;
                itemList.push_back(pRails);
                pRails->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".CREATE") == 0) {
                HMGFileCreate* pCreate = new HMGFileCreate;
                itemList.push_back(pCreate);
                pCreate->Read(reader, line, lineInfo);
            }
            else if (strcmp(token, ".END") == 0) {  // global only
                isStop = true;
            }
            else if (strcmp(token, ".REPLACE") == 0) { // global only
                // az eredeti komponens NameToIndex-eit használja, így akárhány replace után is ugyanarra a névre ugyanazt az indexet adja, 
                // de amúgy üresen indul
                token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
                while (*token == '@')token++;
                while (*token == ':')token++;
                // subcktNames, controllerNames, componentTemplateNames, modelNames
                unsigned index;
/*///                if ((index = globalNames.subcktNames.find(token)) != 0) {
                    SpiceSubcktDescription* psubckt = new SpiceSubcktDescription;
                    itemList.push_back(psubckt);
                    psubckt->Replace(globalNames.subcktData[index], reader, line, lineInfo);
                    //vectorForcedSet(globalNames.subcktData, psubckt, psubckt->subcktIndex); // a tényleges csere majd a szimulációnál lesz
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
*///
            }
            else
                throw hmgExcept("HMGFileGlobalDescription::Read", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else
            throw("HMGFileGlobalDescription::Read", "this line type is not supported yet in Hex Open: %s", line);
    }
}


//***********************************************************************
void SpiceExpression::toInstructionStream(InstructionStream& iStream, unsigned index) {
//***********************************************************************
    iStream.add(new IsExpressionInstruction(index, (unsigned)theExpression.size()));
    for (SpiceExpressionAtom& it : theExpression)
        iStream.add(new IsExpressionAtomInstruction(it));
    iStream.add(new IsEndDefInstruction(sitEndExpression, 0));
}


//***********************************************************************
void HMGFileComponentInstanceLine::toInstructionStream(InstructionStream& iStream) {
//***********************************************************************
    switch (instanceOfWhat) {
        case itModel:                  iStream.add(new IsComponentInstanceInstruction(sitSubcktInstance,           instanceIndex, indexOfTypeInGlobalContainer)); break;
        //case itController:              iStream.add(new IsComponentInstanceInstruction(sitControllerInstance,       instanceIndex, indexOfTypeInGlobalContainer)); break;
        case itSunredTree:              iStream.add(new IsComponentInstanceInstruction(sitSunredTree,               instanceIndex, indexOfTypeInGlobalContainer)); break;
        case itBuiltInComponentType:    iStream.add(new IsComponentInstanceInstruction(sitBuiltInComponentInstance, instanceIndex, indexOfTypeInGlobalContainer)); break;
        default: throw hmgExcept("SpiceComponentInstanceLine::toInstructionStream", "program error bad instanceOfWhat value (%u)", instanceOfWhat);
    }

    iStream.add(new IsSetContainerSizeInstruction(sitNodeValueContainerSize, (unsigned)nodes.size()));
    for (SimpleNodeID& it : nodes) 
        iStream.add(new IsNodeValueInstruction(it));

    iStream.add(new IsSetContainerSizeInstruction(sitParameterValueContainerSize, (unsigned)params.size()));
    for (ParameterInstance& it : params)
        iStream.add(new IsParameterValueInstruction(it));

    //valueExpression.toInstructionStream(iStream, 0);

    iStream.add(new IsEndDefInstruction(sitEndComponentInstance, instanceIndex));
}


//***********************************************************************
void HMGFileModelDescription::toInstructionStream(InstructionStream& iStream) {
//***********************************************************************
    //if (isGlobal) {
    //    if (!globalNames.subcktNames.getIsEmpty())              iStream.add(new IsSetContainerSizeInstruction(sitSetSubcktContainerSize,        globalNames.subcktNames.getLastIndex() + 1));
    //    if (!globalNames.controllerNames.getIsEmpty())          iStream.add(new IsSetContainerSizeInstruction(sitSetControllerContainerSize,    globalNames.controllerNames.getLastIndex() + 1));
    //    if (!globalNames.componentTemplateNames.getIsEmpty())   iStream.add(new IsSetContainerSizeInstruction(sitSetComponentTypeContainerSize, globalNames.componentTemplateNames.getLastIndex() + 1));
    //    if (!globalNames.staticVarNames.getIsEmpty())           iStream.add(new IsSetContainerSizeInstruction(sitSetStaticVarContainerSize,     globalNames.staticVarNames.getLastIndex() + 1));
    //    if (!globalNames.modelNames.getIsEmpty())               iStream.add(new IsSetContainerSizeInstruction(sitSetModelContainerSize,         globalNames.modelNames.getLastIndex() + 1));
    //    if (!globalNames.expressionNames.getIsEmpty())          iStream.add(new IsSetContainerSizeInstruction(sitSetExpressionContainerSize,    globalNames.expressionNames.getLastIndex() + 1));
    //    if (!globalNames.sunredNames.getIsEmpty())              iStream.add(new IsSetContainerSizeInstruction(sitSetSunredContainerSize,        globalNames.sunredNames.getLastIndex() + 1));
    //}
    //else {
        //if(isReplacer)  iStream.add(new IsReplaceInstruction(sitReplaceSubckt, modelIndex));
        //else            iStream.add(new IsDefSubcktInstruction(modelIndex, nNormalNode, nControlNode, nParams));
    //}

    //if (!componentInstanceNames.getIsEmpty())   iStream.add(new IsSetContainerSizeInstruction(sitSetComponentInstanceSize,      componentInstanceNames.getLastIndex() + 1));
    //if (!internalNodeNames.getIsEmpty())        iStream.add(new IsSetContainerSizeInstruction(sitSetInternalNodeContainerSize,  internalNodeNames.getLastIndex() + 1));
    //if (!instanceVarNames.getIsEmpty())         iStream.add(new IsSetContainerSizeInstruction(sitSetVarContainerSize,           instanceVarNames.getLastIndex() + 1));
    //if (!probeNames.getIsEmpty())               iStream.add(new IsSetContainerSizeInstruction(sitSetProbeContainerSize,         probeNames.getLastIndex() + 1));
    //if (!forwardedNames.getIsEmpty())           iStream.add(new IsSetContainerSizeInstruction(sitSetForwardedContainerSize,     forwardedNames.getLastIndex() + 1));

    //for (HMGFileListItem* it : itemList)
    //    it->toInstructionStream(iStream);

    iStream.add(new IsEndDefInstruction(sitEndDefSubckt, modelIndex));
}


}
