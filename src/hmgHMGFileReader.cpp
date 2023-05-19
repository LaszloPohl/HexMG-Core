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

    globalCircuit.Read(reader, line, lineInfo, "");
    most("Read");
    globalCircuit.ProcessXLines();
    most("ProcessXLines");
    globalCircuit.ProcessExpressionNames(&globalCircuit.itemList);
    most("ProcessExpressionNames");
}


//***********************************************************************
void SpiceSubcktDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo, const std::string& containerName) {
//***********************************************************************
    LineTokenizer lineToken;

    // read subcircuit head (if not the global circuit is readed)

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".SUBCKT") != 0)
        throw hmgExcept("SpiceSubcktDescription::Read", ".SUBCKT expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read subckt name

    if (lineToken.isSepEOL || lineToken.isSepOpeningBracket)
        throw hmgExcept("SpiceSubcktDescription::Read", "missing .SUBCKT name in %s, line %u", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("SpiceSubcktDescription::Read", "simple .SUBCKT name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (containerName.length() > 0)
        fullName = containerName + "." + lineToken.getActToken();
    else
        fullName = lineToken.getActToken();
///    if(!globalNames.subcktNames.add(fullName, subcktIndex))
///        throw hmgExcept("SpiceSubcktDescription::Read", ".SUBCKT %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read external node names

    nNormalNode = nControlNode = 0;
    while (!lineToken.isSepEOL && !lineToken.isSepOpeningBracket) {
        if (lineToken.getIsControl()) {
            nControlNode++;
        }
        else {
            if (nControlNode > 0)
                throw hmgExcept("SpiceSubcktDescription::Read", "normal and control nodes cannot be mixed, %s arrived in %s, line %u", line, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            nNormalNode++;
        }
        if(!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
            throw hmgExcept("SpiceSubcktDescription::Read", "simple node name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        token = lineToken.getActToken();
        //if(!externalNodeNames.add(token))
        //    throw hmgExcept("SpiceSubcktDescription::Read", "%s node is redefined in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }
        
    // read parameters

    nParams = 0;
    if (!lineToken.isSepEOL && lineToken.isSepOpeningBracket) { // there are parameters
        while (!lineToken.isSepEOL && !lineToken.isSepClosingBracket) {
            if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
                throw hmgExcept("SpiceSubcktDescription::Read", "simple parameter name expected, %s arrived in %s, line %u", 
                    lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            token = lineToken.getActToken();
            //if (!constParameterNames.add(token))
            //    throw hmgExcept("SpiceSubcktDescription::Read", "%s parameter is redefined in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            nParams++;
        }
        if (!lineToken.isSepEOL && lineToken.isSepClosingBracket)
            throw hmgExcept("SpiceSubcktDescription::Read", "there is something after the closing bracket: %s in %s, line %u", 
                lineToken.getRestOfTheLine(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }

    ReadOrReplaceBody(reader, line, lineInfo, false);
}


//***********************************************************************
void SpiceSubcktDescription::Replace(SpiceSubcktDescription* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    subcktIndex = pParent->subcktIndex;
    fullName = parent->fullName;// +":" + std::to_string(replaceCounter++);
    //if (!globalNames.subcktNames.add(fullName, subcktIndex))
    //    throw hmgExcept("SpiceSubcktDescription::Read", "program error: conflicting replace .SUBCKT names: %s in %s, line %u", fullName.c_str(),
    //        reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    ReadOrReplaceBody(reader, line, lineInfo, true);
}


//***********************************************************************
void SpiceSubcktDescription::ReadOrReplaceBody(ReadALine& reader, char* line, LineInfo& lineInfo, bool) {
//***********************************************************************
    bool isSubcktNotEnded = true;
    LineTokenizer lineToken;

    do {
        if(!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", "incomplete .SUBCKT definition, missing .ENDS in %s", reader.getFileName(lineInfo).c_str());
        if (line[0] == 'X' || line[0] == 'R' || line[0] == 'C' || line[0] == 'V' || line[0] == 'I') {
            SpiceComponentInstanceLine* pxline = new SpiceComponentInstanceLine;
            itemList.push_back(pxline);

            pxline->theLine = line;
            pxline->theLineInfo = lineInfo;
            pxline->fileName = reader.getFileName(lineInfo);

            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            //if (!componentInstanceNames.add(token, pxline->componentInstanceIndex))
            //    throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", "%s redefinition in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            //vectorForcedSet< HMGFileListItem* >(componentInstances, pxline, pxline->componentInstanceIndex);
        }
        else if (line[0] == '.') {
            lineToken.init(line);
            const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            if (strcmp(token, ".SUBCKT") == 0) {
                SpiceSubcktDescription* psubckt = new SpiceSubcktDescription;
                itemList.push_back(psubckt);
                psubckt->Read(reader, line, lineInfo, fullName);
///                vectorForcedSet(globalNames.subcktData, psubckt, psubckt->subcktIndex);
            }
            else if (strcmp(token, ".END") == 0) {  // global only
                    throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", ".END arrived in the body of a .SUBCKT definition in %s, line %u", 
                        reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            }
            else if (strcmp(token, ".ENDS") == 0) { // local only
                    throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", ".ENDS arrived in the global circuit in %s, line %u", 
                        reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            }
            else if (strcmp(token, ".CONTROLLER") == 0) { // full name stored!
                SpiceControllerDescription* pController = new SpiceControllerDescription;
                itemList.push_back(pController);
                pController->Read(reader, line, lineInfo, fullName);
///                vectorForcedSet(globalNames.controllerData, pController, pController->controllerIndex);
            }
            else if (strcmp(token, ".EXPRESSION") == 0) { // full name stored!
                SpiceExpressionLine* pExpression = new SpiceExpressionLine;
                itemList.push_back(pExpression);
                pExpression->Read(reader, line, lineInfo, fullName);
            }
            else if (strcmp(token, ".MODEL") == 0) { // full name stored!
                TODO("SpiceSubcktDescription::ReadOrReplaceBody: .MODEL"); // => .replace should be usable
            }
            else if (strcmp(token, ".REPLACE") == 0) { // global only
                    throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", ".REPLACE arrived in the body of a .SUBCKT definition in %s, line %u", 
                        reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
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
                    psubckt->Replace(globalNames.subcktData[index],reader, line, lineInfo);
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
                    TODO("SpiceSubcktDescription::ReadOrReplaceBody: .REPLACE .MODEL");
                }
                else
                    throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", "unrecognised .REPLACE name (%s) arrived in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
*///
            }
            else
                throw hmgExcept("SpiceSubcktDescription::ReadOrReplaceBody", "unrecognised token (%s) in %s, line %u: %s", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        }
        else
            throw("SpiceSubcktDescription::ReadOrReplaceBody", "this line type is not supported yet in Hex Open: %s", line);
    } while (isSubcktNotEnded);
}


//***********************************************************************
void SpiceSubcktDescription::ProcessXLines() {
//***********************************************************************
    LineTokenizer lineToken;
    const char* token = nullptr;

    for (HMGFileListItem* pItem : itemList) {
        if (pItem->getItemType() == itSubckt)
            static_cast<SpiceSubcktDescription*>(pItem)->ProcessXLines();
        else if (pItem->getItemType() == itComponentInstance) { // normal nodes
            SpiceComponentInstanceLine* pInstance = static_cast<SpiceComponentInstanceLine*>(pItem);
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

/*///                    if ((pInstance->indexOfTypeInGlobalContainer = globalNames.componentTemplateNames.findMultiLevel(fullName.length() > 0 ? fullName + "." + token : token)) != 0) {
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
*///                    
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
            SpiceComponentInstanceLine* pInstance = static_cast<SpiceComponentInstanceLine*>(pItem);

            lineToken.init(pInstance->theLine.c_str());

            lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // name ignored

            unsigned parentVectorIndex = 0;
            for (unsigned i = 0; i < pInstance->nNormalNode; i++, parentVectorIndex++) {
                lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine); // normal nodes ignored
            }

            for (unsigned i = 0; i < pInstance->nControlNode; i++, parentVectorIndex++) {
                token = lineToken.getNextToken(pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine);
/*///                if (!identifyNode(pInstance->nodes[parentVectorIndex], token)) {
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
*///
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
}


//***********************************************************************
void SpiceSubcktDescription::ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList) {
//***********************************************************************
    for (HMGFileListItem* pItem : (pItemList== nullptr ? itemList : *pItemList)) {
        switch (pItem->getItemType()) {
            case itNone: break;
            case itComponentInstance: {
                    SpiceComponentInstanceLine* pInstance = static_cast<SpiceComponentInstanceLine*>(pItem);
                    for (auto& it : pInstance->valueExpression.theExpression) {
///                        if (!identifyExpressionNodeOrPar(it))
///                            throw hmgExcept("SpiceComponentTemplateLine::ProcessExpressionNames", "unknown identifier (%s) in %s, line %u: %s", 
///                                it.name.c_str(), pInstance->fileName.c_str(), pInstance->theLineInfo.firstLine, pInstance->theLine.c_str());
                    }
                }
                break;
            case itSubckt: static_cast<SpiceSubcktDescription*>(pItem)->ProcessExpressionNames(nullptr); break;
///            case itComponentTemplate: static_cast<SpiceComponentTemplateLine*>(pItem)->ProcessExpressionNames(); break;
            case itBuiltInComponentType: break;
            case itController: static_cast<SpiceControllerDescription*>(pItem)->ProcessExpressionNames(nullptr); break;
            case itExpression: {
                    SpiceExpressionLine* pExpression = static_cast<SpiceExpressionLine*>(pItem);
                    for (auto& it : pExpression->theExpression.theExpression) {
///                        if (!identifyExpressionNodeOrPar(it))
///                            throw hmgExcept("SpiceComponentTemplateLine::ProcessExpressionNames", "unknown identifier (%s) in %s", it.name.c_str(), pExpression->fullName.c_str());
                    }
                }
                break;
        }
    }
}


//***********************************************************************
void HMGFileSunredTree::Read(ReadALine& reader, char* line, LineInfo& lineInfo, const std::string&) {
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

}


//***********************************************************************
void HMGFileGlobalDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo, const std::string&) {
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

                SpiceSubcktDescription* psubckt = new SpiceSubcktDescription;
                itemList.push_back(psubckt);
                psubckt->Read(reader, line, lineInfo, fullName);
///                vectorForcedSet(globalNames.subcktData, psubckt, psubckt->subcktIndex);
            }
            else if (strcmp(token, ".SUNREDTREE") == 0) {

                HMGFileSunredTree* psunredtree = new HMGFileSunredTree;
                itemList.push_back(psunredtree);
                psunredtree->Read(reader, line, lineInfo, fullName);
                ///                vectorForcedSet(globalNames.subcktData, psubckt, psubckt->subcktIndex);
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
void SpiceComponentInstanceLine::toInstructionStream(InstructionStream& iStream) {
//***********************************************************************
    switch (instanceOfWhat) {
        case itSubckt:                  iStream.add(new IsComponentInstanceInstruction(sitSubcktInstance,           componentInstanceIndex, indexOfTypeInGlobalContainer)); break;
        case itController:              iStream.add(new IsComponentInstanceInstruction(sitControllerInstance,       componentInstanceIndex, indexOfTypeInGlobalContainer)); break;
        case itSunredTree:              iStream.add(new IsComponentInstanceInstruction(sitSunredTree,               componentInstanceIndex, indexOfTypeInGlobalContainer)); break;
        case itBuiltInComponentType:    iStream.add(new IsComponentInstanceInstruction(sitBuiltInComponentInstance, componentInstanceIndex, indexOfTypeInGlobalContainer)); break;
        default: throw hmgExcept("SpiceComponentInstanceLine::toInstructionStream", "program error bad instanceOfWhat value (%u)", instanceOfWhat);
    }

    iStream.add(new IsSetContainerSizeInstruction(sitNodeValueContainerSize, (unsigned)nodes.size()));
    for (NodeID& it : nodes) 
        iStream.add(new IsNodeValueInstruction(it));

    iStream.add(new IsSetContainerSizeInstruction(sitParameterValueContainerSize, (unsigned)params.size()));
    for (ParameterInstance& it : params)
        iStream.add(new IsParameterValueInstruction(it));

    valueExpression.toInstructionStream(iStream, 0);

    iStream.add(new IsEndDefInstruction(sitEndComponentInstance, componentInstanceIndex));
}


//***********************************************************************
void SpiceSubcktDescription::toInstructionStream(InstructionStream& iStream) {
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
        if(isReplacer)  iStream.add(new IsReplaceInstruction(sitReplaceSubckt, subcktIndex));
        else            iStream.add(new IsDefSubcktInstruction(subcktIndex, nNormalNode, nControlNode, nParams));
    //}

    //if (!componentInstanceNames.getIsEmpty())   iStream.add(new IsSetContainerSizeInstruction(sitSetComponentInstanceSize,      componentInstanceNames.getLastIndex() + 1));
    //if (!internalNodeNames.getIsEmpty())        iStream.add(new IsSetContainerSizeInstruction(sitSetInternalNodeContainerSize,  internalNodeNames.getLastIndex() + 1));
    //if (!instanceVarNames.getIsEmpty())         iStream.add(new IsSetContainerSizeInstruction(sitSetVarContainerSize,           instanceVarNames.getLastIndex() + 1));
    //if (!probeNames.getIsEmpty())               iStream.add(new IsSetContainerSizeInstruction(sitSetProbeContainerSize,         probeNames.getLastIndex() + 1));
    //if (!forwardedNames.getIsEmpty())           iStream.add(new IsSetContainerSizeInstruction(sitSetForwardedContainerSize,     forwardedNames.getLastIndex() + 1));

    for (HMGFileListItem* it : itemList)
        it->toInstructionStream(iStream);

    iStream.add(new IsEndDefInstruction(sitEndDefSubckt, subcktIndex));
}


//***********************************************************************
void SpiceControllerDescription::Read(ReadALine& reader, char* line, LineInfo& lineInfo, const std::string& containerName) {
//***********************************************************************
    LineTokenizer lineToken;

    // read controller head

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // check line

    if (strcmp(token, ".CONTROLLER") != 0)
        throw hmgExcept("SpiceControllerDescription::Read", ".CONTROLLER expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read controller name

    if (lineToken.isSepEOL || lineToken.isSepOpeningBracket)
        throw hmgExcept("SpiceControllerDescription::Read", "missing .CONTROLLER name in %s, line %u", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("SpiceControllerDescription::Read", "simple .CONTROLLER name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (containerName.length() > 0)
        fullName = containerName + "." + lineToken.getActToken();
    else
        fullName = lineToken.getActToken();
///    if (!globalNames.controllerNames.add(fullName, controllerIndex))
///        throw hmgExcept("SpiceControllerDescription::Read", ".CONTROLLER %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read external node names

    nControlNode = 0;
    while (!lineToken.isSepEOL && !lineToken.isSepOpeningBracket) {
        lineToken.getIsControl(); // whether marked or not, all nodes are control
        nControlNode++;
        if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
            throw hmgExcept("SpiceControllerDescription::Read", "simple node name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        token = lineToken.getActToken();
        //if (!externalNodeNames.add(token))
        //    throw hmgExcept("SpiceControllerDescription::Read", "%s node is redefined in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }

    // read parameters

    nParams = 0;
    if (!lineToken.isSepEOL && lineToken.isSepOpeningBracket) { // there are parameters
        while (!lineToken.isSepEOL && !lineToken.isSepClosingBracket) {
            if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
                throw hmgExcept("SpiceControllerDescription::Read", "simple parameter name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            token = lineToken.getActToken();
            //if (!constParameterNames.add(token))
            //    throw hmgExcept("SpiceControllerDescription::Read", "%s parameter is redefined in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
            nParams++;
        }
        if (!lineToken.isSepEOL && lineToken.isSepClosingBracket)
            throw hmgExcept("SpiceControllerDescription::Read", "there is something after the closing bracket: %s in %s, line %u", lineToken.getRestOfTheLine(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    }

    ReadOrReplaceBody(reader, line, lineInfo, false);
}


//***********************************************************************
void SpiceControllerDescription::Replace(SpiceControllerDescription* parent, ReadALine& reader, char* line, LineInfo& lineInfo) {
//***********************************************************************
    isReplacer = true;
    pParent = parent;
    controllerIndex = pParent->controllerIndex;
    fullName = parent->fullName; // +":" + std::to_string(replaceCounter++);
    //if (!globalNames.controllerNames.add(fullName, controllerIndex))
    //    throw hmgExcept("SpiceControllerDescription::Read", "program error: conflicting replace .SUBCKT names: %s in %s, line %u", fullName.c_str(),
    //        reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    ReadOrReplaceBody(reader, line, lineInfo, true);
}


//***********************************************************************
void SpiceControllerDescription::ReadOrReplaceBody(ReadALine& reader, char* line, LineInfo& lineInfo, bool) {
//***********************************************************************
    bool isControllerNotEnded = true;
    LineTokenizer lineToken;

    do {
        if (!reader.getLine(line, MAX_LINE_LENGHT, lineInfo))
            throw hmgExcept("SpiceControllerDescription::ReadOrReplaceBody", "incomplete .CONTROLLER definition, missing .ENDC in %s", reader.getFileName(lineInfo).c_str());
        if(line[0] != '.')
            throw hmgExcept("SpiceControllerDescription::ReadOrReplaceBody", "only . started control instructions allowed in a controller in %s, line %u: %s", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, line);
        lineToken.init(line);
        const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
        if (strcmp(token, ".EXPRESSION") == 0) { // full name stored!
            SpiceExpressionLine* pExpression = new SpiceExpressionLine;
            itemList.push_back(pExpression);
            pExpression->Read(reader, line, lineInfo, fullName);
        }
        else
            throw hmgExcept("SpiceControllerDescription::ReadOrReplaceBody", "unrecognised token (%s) in %s", token, reader.getFileName(lineInfo).c_str());
    } while (isControllerNotEnded);
}


//***********************************************************************
void SpiceControllerDescription::ProcessExpressionNames(std::list< HMGFileListItem* >* pItemList) {
//***********************************************************************
    for (HMGFileListItem* pItem : (pItemList== nullptr ? itemList : *pItemList)) {
        switch (pItem->getItemType()) {
            case itNone: break;
            case itComponentInstance: break;
            case itSubckt: break;
            case itSunredTree: break;
            case itBuiltInComponentType: break;
            case itController: break;
            case itExpression: {
                    SpiceExpressionLine* pExpression = static_cast<SpiceExpressionLine*>(pItem);
                    for (auto& it : pExpression->theExpression.theExpression) {
///                        if (!identifyExpressionNodeOrPar(it))
///                            throw hmgExcept("SpiceControllerDescription::ProcessExpressionNames", "unknown identifier (%s) in %s", it.name.c_str(), pExpression->fullName.c_str());
                    }
                }
                break;
        }
    }
}


//***********************************************************************
void SpiceControllerDescription::toInstructionStream(InstructionStream& iStream) {
//***********************************************************************
    if (isReplacer) iStream.add(new IsReplaceInstruction(sitReplaceController, controllerIndex));
    else            iStream.add(new IsDefControllerInstruction(controllerIndex, nControlNode, nParams));

    //if (!instanceVarNames.getIsEmpty()) iStream.add(new IsSetContainerSizeInstruction(sitSetVarContainerSize, instanceVarNames.getLastIndex() + 1));

    for (HMGFileListItem* it : itemList)
        it->toInstructionStream(iStream);

    iStream.add(new IsEndDefInstruction(sitEndDefController, controllerIndex));
}


//***********************************************************************
void SpiceExpressionLine::Read(ReadALine& reader, char* line, LineInfo& lineInfo, const std::string& containerName) {
//***********************************************************************
    LineTokenizer lineToken;

    lineToken.init(line);
    const char* token = lineToken.getNextToken(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (strcmp(token, ".EXPRESSION") != 0)
        throw hmgExcept("SpiceExpressionLine::Read", ".EXPRESSION expected, %s found in %s, line %u", token, reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // read expression name

    if (lineToken.isSepEOL || lineToken.isSepOpeningBracket || lineToken.isSepComma)
        throw hmgExcept("SpiceExpressionLine::Read", "missing .EXPRESSION name in %s, line %u", reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (!lineToken.getNextTokenSimple(reader.getFileName(lineInfo).c_str(), lineInfo.firstLine))
        throw hmgExcept("SpiceExpressionLine::Read", "simple .EXPRESSION name expected, %s arrived in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);
    if (containerName.length() > 0)
        fullName = containerName + "." + lineToken.getActToken();
    else
        fullName = lineToken.getActToken();
///    if (!globalNames.expressionNames.add(fullName, expressionIndex))
///        throw hmgExcept("SpiceExpressionLine::Read", ".EXPRESSION %s redefinition in %s, line %u", lineToken.getActToken(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine);

    // the expression

    token = lineToken.getRestOfTheLine();
    if (!theExpression.buildFromString(token))
        throw hmgExcept("SpiceExpressionLine::Read", "invalid expression: %s in %s, line %u: %s)", 
            theExpression.errorMessage.c_str(), reader.getFileName(lineInfo).c_str(), lineInfo.firstLine, lineToken.getLine());
}


}
