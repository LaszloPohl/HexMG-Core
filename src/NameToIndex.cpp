//***********************************************************************
// Hex Open Name to Index Source
// Creation date:  2021. 06. 29.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#include "NameToIndex.h"
#include <list>
#include <cmath>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
unsigned short SpiceExpression::addToTheExpression(const SpiceExpressionAtom& actAtom) {
//***********************************************************************
    ExpressionAndComponentType type = actAtom.atomType;
    for (unsigned i = 1; i < theExpression.size(); i++) {
        if (theExpression[i].atomType == type) {
            switch (type) {
                case etConst:       if (theExpression[i].constValue         == actAtom.constValue)    return unsigned short(i); break;
                case etFunction:    if (theExpression[i].functionType       == actAtom.functionType 
                                        && theExpression[i].par1OrSourceIndex == actAtom.par1OrSourceIndex
                                        && theExpression[i].par2OrPrevIndex == actAtom.par2OrPrevIndex)
                                        return unsigned short(i);
                                    break;
                case etListElem:    if (theExpression[i].par1OrSourceIndex  == actAtom.par1OrSourceIndex
                                        && theExpression[i].par2OrPrevIndex == actAtom.par2OrPrevIndex)
                                        return unsigned short(i);
                                    break;
                case etNodeOrVar:   if (theExpression[i].name               == actAtom.name)                return unsigned short(i); break;
            }
        }
    }
    theExpression.push_back(actAtom);
    return (unsigned short)(theExpression.size() - 1);
    return 0;
}


//***********************************************************************
bool SpiceExpression::buildFromString(const char* spiceExpression) {
//***********************************************************************
    theExpression.clear();
    SpiceExpressionAtom actAtom;
    theExpression.push_back(actAtom); // index 0 is invalid
    if (errorMessage.length() != 0)errorMessage.clear();

    // create token list

    std::list<LineTokenizer::ExpressionToken> tokenList;
    LineTokenizer tokenizer;
    tokenizer.init(spiceExpression);
    while(!tokenizer.isSepEOL){
        tokenList.push_back(tokenizer.getNextExpressionToken());
        if (tokenList.back().exprType == LineTokenizer::ExpressionToken::etError) {
            errorMessage = tokenizer.errorMessage;
            return false;
        }
    }

    if (tokenList.begin() == tokenList.end())
        return true;

    // signs

    for (auto it = tokenList.begin(); it != tokenList.end();) {
        if (it->exprType == LineTokenizer::ExpressionToken::etOperator
            && (it->opType == LineTokenizer::ExpressionToken::otPlus
                || it->opType == LineTokenizer::ExpressionToken::otMinus)) {
            bool isSign = false;
            if (it == tokenList.begin())
                isSign = true;
            else {
                --it;
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator
                    && it->opType != LineTokenizer::ExpressionToken::otCloseBracket) {
                    isSign = true;
                }
                ++it;
            }
            if (isSign) {
                if(it->opType == LineTokenizer::ExpressionToken::otPlus) // an unary + sign does nothing, it is deleted
                    it = tokenList.erase(it);
                else { // unary - sign
                    ++it;
                    if (it == tokenList.end()) {
                        errorMessage = "expression ends after \'-\'";
                        return false;
                    }
                    if (it->exprType == LineTokenizer::ExpressionToken::etNumber) {
                        it->value = -it->value;
                        --it;
                        it = tokenList.erase(it);
                    }
                    else if (it->exprType == LineTokenizer::ExpressionToken::etOperator // -+ or --
                        && (it->opType == LineTokenizer::ExpressionToken::otPlus
                            || it->opType == LineTokenizer::ExpressionToken::otMinus)) {
                        if (it->opType == LineTokenizer::ExpressionToken::otPlus)
                            it->opType = LineTokenizer::ExpressionToken::otMinus;
                        else
                            it->opType = LineTokenizer::ExpressionToken::otPlus;
                        --it;
                        it = tokenList.erase(it);
                    }
                    else {
                        --it;
                        it->opType = LineTokenizer::ExpressionToken::otMinusSign;
                        ++it;
                    }
                }
            }
            else ++it;
        }
        else ++it;
    }

    // inv(number)

    for (auto it = tokenList.begin(); it != tokenList.end(); ++it) {
        if (it->exprType == LineTokenizer::ExpressionToken::etName
            && (it->name == ".INV" || it->name == "INV")) {
            ++it;
            if (it != tokenList.end() && it->exprType == LineTokenizer::ExpressionToken::etOperator
                && it->opType == LineTokenizer::ExpressionToken::otOpenBracket) {
                ++it;
                if (it != tokenList.end() && it->exprType == LineTokenizer::ExpressionToken::etNumber) {
                    ++it;
                    if (it != tokenList.end() && it->exprType == LineTokenizer::ExpressionToken::etOperator
                        && it->opType == LineTokenizer::ExpressionToken::otCloseBracket) {
                        it = tokenList.erase(it); // closing bracket
                        --it; // the number
                        it->value = (it->value == 0) ? 1e20 : (1 / it->value);
                        --it;
                        it = tokenList.erase(it); // opening bracket
                        --it;
                        it = tokenList.erase(it); // .inv / inv
                    }
                    else
                        --it;
                }
                else
                    --it;
            }
            else
                --it;
        }
    }

    // check opening and closing brackets

    int bracketDepth = 0;
    for (auto it = tokenList.begin(); it != tokenList.end(); ++it) {
        if (it->exprType == LineTokenizer::ExpressionToken::etOperator) {
            if (it->opType == LineTokenizer::ExpressionToken::otOpenBracket)
                bracketDepth++;
            else if (it->opType == LineTokenizer::ExpressionToken::otCloseBracket)
                bracketDepth--;
            if (bracketDepth < 0) {
                errorMessage = "more closing bracket than opening";
                return false;
            }
        }
    }
    if (bracketDepth != 0) {
        errorMessage = "more closing bracket than opening";
        return false;
    }

    // move elementary blocks (constants, nodes, variables) to theExpression

    {
        auto it1 = tokenList.begin();
        auto it2 = tokenList.begin();
        auto it3 = tokenList.begin();
        ++it2;

        // there is one item in the token list

        if (it2 == tokenList.end()) {
            if (it1->exprType == LineTokenizer::ExpressionToken::etOperator) {
                errorMessage = "expression conatins only an operator";
                return false;
            }
            if (it1->exprType == LineTokenizer::ExpressionToken::etNumber) {
                actAtom.reset(etConst);
                actAtom.isConst = true;
                actAtom.constValue = it1->value;
                theExpression.push_back(actAtom);
                return true;
            }
            if (it1->exprType == LineTokenizer::ExpressionToken::etName) {
                actAtom.reset(etNodeOrVar);
                actAtom.name = it1->name;
                theExpression.push_back(actAtom);
                return true;
            }
            errorMessage = "invalid expression";
            return false;
        }

        // at least two items

        ++it3;
        if(it3!= tokenList.end())
            ++it3;

        // the starting token

        if (it1->exprType == LineTokenizer::ExpressionToken::etNumber || it1->exprType == LineTokenizer::ExpressionToken::etName) {
            if (it2->exprType != LineTokenizer::ExpressionToken::etOperator) { // number or name must be followed by operator
                errorMessage = "no operator between expression items";
                return false;
            }
            if (it2->opType != LineTokenizer::ExpressionToken::otOpenBracket) {
                if (it1->exprType == LineTokenizer::ExpressionToken::etNumber) {
                    actAtom.reset(etConst);
                    actAtom.isConst = true;
                    actAtom.constValue = it1->value;
                    theExpression.push_back(actAtom);
                    it1->exprType = LineTokenizer::ExpressionToken::etStoredToken;
                    it1->storedTokenIndex = (unsigned short)(theExpression.size() - 1);
                }
                else { // name
                    actAtom.reset(etNodeOrVar);
                    actAtom.name = it1->name;
                    theExpression.push_back(actAtom);
                    it1->exprType = LineTokenizer::ExpressionToken::etStoredToken;
                    it1->storedTokenIndex = (unsigned short)(theExpression.size() - 1);
                }
            }
            else if (it1->exprType == LineTokenizer::ExpressionToken::etNumber) { // a function name is not a number
                errorMessage = "number as function name";
                return false;
            }
        }

        // the remaining tokens

        while (it2 != tokenList.end()) {
            if (it2->exprType == LineTokenizer::ExpressionToken::etNumber || it2->exprType == LineTokenizer::ExpressionToken::etName) {
                if (it3 != tokenList.end() && it3->exprType != LineTokenizer::ExpressionToken::etOperator) { // number or name must be followed by operator
                    errorMessage = "no operator between expression items";
                    return false;
                }
                if (it3 == tokenList.end() || it3->opType != LineTokenizer::ExpressionToken::otOpenBracket) {
                    if (it2->exprType == LineTokenizer::ExpressionToken::etNumber) {
                        actAtom.reset(etConst);
                        actAtom.isConst = true;
                        actAtom.constValue = it2->value;
                        it2->exprType = LineTokenizer::ExpressionToken::etStoredToken;
                        it2->storedTokenIndex = addToTheExpression(actAtom);
                    }
                    else { // name
                        actAtom.reset(etNodeOrVar);
                        actAtom.name = it2->name;
                        it2->exprType = LineTokenizer::ExpressionToken::etStoredToken;
                        it2->storedTokenIndex = addToTheExpression(actAtom);
                    }
                }
            }
            else if (it3 != tokenList.end() && it1->exprType == LineTokenizer::ExpressionToken::etNumber) { // a function name is not a number
                errorMessage = "number as function name";
                return false;
            }
            it1 = it2;
            it2 = it3;
            if (it3 != tokenList.end())
                it3++;
        }
    }

    while (tokenList.begin() != tokenList.end()) {

        // processing brackets

        auto start = tokenList.begin();
        auto stop  = tokenList.end();
        for (auto it = tokenList.begin(); it != tokenList.end(); ++it) {
            if (it->exprType == LineTokenizer::ExpressionToken::etOperator) {
                if (it->opType == LineTokenizer::ExpressionToken::otOpenBracket)
                    start = it;
                else if (it->opType == LineTokenizer::ExpressionToken::otCloseBracket) {
                    stop = it;
                    ++start; // start points to the first item after the bracket
                    break;
                }
            }
        }

        if (start != stop) { // at least one token is in the start-stop range

            // now start points the first token after an openig bracket, or to the tokenList.begin(), if no bracket
            //     stop  points the closing bracket belongs to the opening bracket before start, or to tokenList.end(), if no bracket
            // now no bracket between start and stop, the operations are processed in precedence order
            // operator precedence:
            //                      -, !
            //                      ^
            //                      *, /
            //                      +, -
            //                      <, <=, >, >=
            //                      ==, !=
            //                      &&
            //                      ||
            //                      ,

            // ! and unary -

            for (auto it = start; it != stop; ++it) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator) {
                    bool isStart = false;
                    if (it->opType == LineTokenizer::ExpressionToken::otNot) {
                        isStart = it == start;
                        auto operandus = it;
                        ++operandus;
                        if (operandus == stop) {
                            errorMessage = "\'!\' in invalid place";
                            return false;
                        }
                        while (it != stop && it->exprType == LineTokenizer::ExpressionToken::etOperator && it->opType == LineTokenizer::ExpressionToken::otNot
                            && operandus->exprType == LineTokenizer::ExpressionToken::etOperator && operandus->opType == LineTokenizer::ExpressionToken::otNot) { // !!(...)
                            it = tokenList.erase(it);
                            operandus = it = tokenList.erase(it);
                            if (it != stop)
                                ++operandus;
                        }
                        if (it != stop && it->exprType == LineTokenizer::ExpressionToken::etOperator && it->opType == LineTokenizer::ExpressionToken::otNot) {
                            if (operandus == stop || operandus->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                                errorMessage = "\'!\' in invalid place";
                                return false; // there is no other unary logic operator, numbers and names are saved
                            }
                            if (!theExpression[operandus->storedTokenIndex].isBool) {
                                errorMessage = "\'!\' used in arithmetic expression";
                                return false;
                            }
                            actAtom.reset(etFunction);
                            actAtom.functionType = futOpNot;
                            actAtom.isBool = true;
                            actAtom.isConst = theExpression[operandus->storedTokenIndex].isConst;
                            if (actAtom.isConst) actAtom.constValue = !theExpression[operandus->storedTokenIndex].constValue;
                            actAtom.par1OrSourceIndex = operandus->storedTokenIndex;
                            it = tokenList.erase(it); // "it" will point to the operandus which is already etStoredToken
                            it->storedTokenIndex = addToTheExpression(actAtom);
                        }
                    }
                    else if (it->opType == LineTokenizer::ExpressionToken::otMinusSign) {
                        isStart = it == start;
                        auto operandus = it;
                        ++operandus;
                        // -- etc. is already processed
                        if (operandus == stop || operandus->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                            errorMessage = "\'-\' sign in invalid place";
                            return false; // there is no other unary arithmetic operator, numbers and names are saved
                        }
                        if (theExpression[operandus->storedTokenIndex].isBool) {
                            errorMessage = "\'-\' sign used in logic expression";
                            return false;
                        }
                        actAtom.reset(etFunction);
                        actAtom.functionType = futOpNegativeSign;
                        actAtom.isConst = theExpression[operandus->storedTokenIndex].isConst;
                        if (actAtom.isConst) actAtom.constValue = -theExpression[operandus->storedTokenIndex].constValue;
                        actAtom.par1OrSourceIndex = operandus->storedTokenIndex;
                        it = tokenList.erase(it); // "it" will point to the operandus which is already etStoredToken
                        it->storedTokenIndex = addToTheExpression(actAtom);
                    }
                    if (isStart)start = it;
                }
            }

            // ^

            auto op1 = start;
            auto it = start;
            ++it;
            auto op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator && op1->opType == LineTokenizer::ExpressionToken::otPow) {
                errorMessage = "\'^\' missing first operandus";
                return false; // missing first operandus
            }

            while(it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator && it->opType == LineTokenizer::ExpressionToken::otPow) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'^\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (opat1.isBool || opat2.isBool) {
                        errorMessage = "\'^\' sign used with logic operandus value";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    if (opat2.isConst && opat2.constValue == 2) {
                        actAtom.functionType = futSquare;
                        if (actAtom.isConst)
                            actAtom.constValue = opat1.constValue * opat1.constValue;
                    }
                    else {
                        actAtom.functionType = futPow;
                        if (actAtom.isConst)
                            actAtom.constValue = pow(opat1.constValue, opat2.constValue);
                        actAtom.par2OrPrevIndex   = op2->storedTokenIndex; // in case of futSquare this is ignored
                    }
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // * /

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator
                && (op1->opType == LineTokenizer::ExpressionToken::otMul || op1->opType == LineTokenizer::ExpressionToken::otDiv)) {
                errorMessage = "\'*\' or \'/\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator 
                    && (it->opType == LineTokenizer::ExpressionToken::otMul || it->opType == LineTokenizer::ExpressionToken::otDiv)) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'*\' or \'/\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (opat1.isBool || opat2.isBool) {
                        errorMessage = "\'*\' or \'/\' used with logic operandus value";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    if (it->opType == LineTokenizer::ExpressionToken::otMul) {
                        actAtom.functionType = futOpMul;
                        if (actAtom.isConst) actAtom.constValue = opat1.constValue * opat2.constValue;
                    }
                    else {
                        actAtom.functionType = futOpDiv;
                        if (opat2.constValue == 0) {
                            errorMessage = "division by zero";
                            return false;
                        }
                        if (actAtom.isConst) actAtom.constValue = opat1.constValue / opat2.constValue;
                    }
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex   = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // + -

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator
                && (op1->opType == LineTokenizer::ExpressionToken::otPlus || op1->opType == LineTokenizer::ExpressionToken::otMinus)) {
                errorMessage = "\'+\' or \'-\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator
                    && (it->opType == LineTokenizer::ExpressionToken::otPlus || it->opType == LineTokenizer::ExpressionToken::otMinus)) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'+\' or \'-\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (opat1.isBool || opat2.isBool) {
                        errorMessage = "\'+\' or \'-\' used with logic operandus value";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    if (it->opType == LineTokenizer::ExpressionToken::otPlus) {
                        actAtom.functionType = futOpPlus;
                        if (actAtom.isConst) actAtom.constValue = opat1.constValue + opat2.constValue;
                    }
                    else {
                        actAtom.functionType = futOpMinus;
                        if (actAtom.isConst) actAtom.constValue = opat1.constValue - opat2.constValue;
                    }
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // < <= > >=

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator
                && (op1->opType == LineTokenizer::ExpressionToken::otSmaller || op1->opType == LineTokenizer::ExpressionToken::otSmEq
                    || op1->opType == LineTokenizer::ExpressionToken::otGreater || op1->opType == LineTokenizer::ExpressionToken::otGrEq)) {
                errorMessage = "\'<\' or \'<=\' or \'>\' or \'>=\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator
                    && (it->opType == LineTokenizer::ExpressionToken::otSmaller || it->opType == LineTokenizer::ExpressionToken::otSmEq
                        || it->opType == LineTokenizer::ExpressionToken::otGreater || it->opType == LineTokenizer::ExpressionToken::otGrEq)) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'<\' or \'<=\' or \'>\' or \'>=\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (opat1.isBool || opat2.isBool) {
                        errorMessage = "\'<\' or \'<=\' or \'>\' or \'>=\' used with logic operandus value, only arithmetic values can be compared";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isBool = true;
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    switch (it->opType) {
                        case LineTokenizer::ExpressionToken::otSmaller: 
                            actAtom.functionType = futOpSmaller;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue < opat2.constValue;
                            break;
                        case LineTokenizer::ExpressionToken::otSmEq:
                            actAtom.functionType = futOpSmEq;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue <= opat2.constValue;
                            break;
                        case LineTokenizer::ExpressionToken::otGreater:
                            actAtom.functionType = futOpGreater;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue > opat2.constValue;
                            break;
                        case LineTokenizer::ExpressionToken::otGrEq:
                            actAtom.functionType = futOpGrEq;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue >= opat2.constValue;
                            break;
                        default:
                            throw hmgExcept("SpiceExpression::buildFromString", "Unexpected it->opType (%u)", it->opType);
                    }
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex   = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // == !=

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator
                && (op1->opType == LineTokenizer::ExpressionToken::otEqual || op1->opType == LineTokenizer::ExpressionToken::otNonEqual)) {
                errorMessage = "\'==\' or \'!=\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator
                    && (it->opType == LineTokenizer::ExpressionToken::otEqual || it->opType == LineTokenizer::ExpressionToken::otNonEqual)) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'==\' or \'!=\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (opat1.isBool || opat2.isBool) {
                        errorMessage = "\'==\' or \'!=\' used with logic operandus value, only arithmetic values can be compared";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isBool = true;
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    switch (it->opType) {
                        case LineTokenizer::ExpressionToken::otEqual:
                            actAtom.functionType = futOpEqual;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue == opat2.constValue;
                            break;
                        case LineTokenizer::ExpressionToken::otNonEqual:
                            actAtom.functionType = futOpNonEqual;
                            if (actAtom.isConst) actAtom.constValue = opat1.constValue != opat2.constValue;
                            break;
                        default:
                            throw hmgExcept("SpiceExpression::buildFromString", "Unexpected it->opType (%u)", it->opType);
                    }
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex   = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // &&

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator && op1->opType == LineTokenizer::ExpressionToken::otAnd) {
                errorMessage = "\'&\' or \'&&\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator && it->opType == LineTokenizer::ExpressionToken::otAnd) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'&\' or \'&&\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (!opat1.isBool || !opat2.isBool) {
                        errorMessage = "\'&\' or \'&&\' used with arithmetic operandus value";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isBool = true;
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    actAtom.functionType = futOpAnd;
                    if (actAtom.isConst) actAtom.constValue = opat1.constValue && opat2.constValue;
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex   = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // ||

            op1 = start;
            it = start;
            ++it;
            op2 = it;
            if (it != stop)
                ++op2;

            if (op1->exprType == LineTokenizer::ExpressionToken::etOperator && op1->opType == LineTokenizer::ExpressionToken::otOr) {
                errorMessage = "\'|\' or \'||\' missing first operandus";
                return false; // missing first operandus
            }

            while (it != stop) {
                if (it->exprType == LineTokenizer::ExpressionToken::etOperator && it->opType == LineTokenizer::ExpressionToken::otOr) {
                    if (op2 == stop || op1->exprType != LineTokenizer::ExpressionToken::etStoredToken
                        || op2->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                        errorMessage = "\'|\' or \'||\' missing second operandus or bad first or second operandus type";
                        return false; // missing second operandus or bad first / second operandus type
                    }
                    const SpiceExpressionAtom& opat1 = theExpression[op1->storedTokenIndex];
                    const SpiceExpressionAtom& opat2 = theExpression[op2->storedTokenIndex];
                    if (!opat1.isBool || !opat2.isBool) {
                        errorMessage = "\'|\' or \'||\' used with arithmetic operandus value";
                        return false;
                    }
                    actAtom.reset(etFunction);
                    actAtom.isBool = true;
                    actAtom.isConst = opat1.isConst && opat2.isConst;
                    actAtom.functionType = futOpOr;
                    if (actAtom.isConst) actAtom.constValue = opat1.constValue || opat2.constValue;
                    actAtom.par1OrSourceIndex = op1->storedTokenIndex;
                    actAtom.par2OrPrevIndex   = op2->storedTokenIndex;
                    bool isStartDeleted = op1 == start;
                    tokenList.erase(op1);
                    op1 = it = tokenList.erase(it); // "it" will point to op2 which is already etStoredToken
                    it->storedTokenIndex = addToTheExpression(actAtom);
                    if (isStartDeleted)
                        start = it;
                    ++it;
                    op2 = it;
                    if (op2 != stop)
                        ++op2;
                }
                else {
                    op1 = it;
                    it = op2;
                    if (op2 != stop)
                        ++op2;
                }
            }

            // ,

            unsigned commaCount = 0;
            for (it = start; it != stop;) {
                if (it->exprType != LineTokenizer::ExpressionToken::etStoredToken) {
                    errorMessage = "invalid expression (StoredToken expected during \',\' parsing)";
                    return false;
                }
                ++it;
                if (it != stop) {
                    if (it->exprType != LineTokenizer::ExpressionToken::etOperator || it->opType != LineTokenizer::ExpressionToken::otComma) {
                        errorMessage = "invalid expression, \',\' expected";
                        return false;
                    }
                    commaCount++;
                    ++it;
                }
            }

            if (commaCount == 0) {
                if (start == tokenList.begin()) {
                    if(tokenList.erase(start)!=tokenList.end())
                        throw hmgExcept("SpiceExpression::buildFromString", "Unexpected tokenList.erase(start)!=tokenList.end()");
                }
                else { // one token in brackets
                    it = start;
                    --it;
                    if (it->exprType != LineTokenizer::ExpressionToken::etOperator || it->opType != LineTokenizer::ExpressionToken::otOpenBracket)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otOpenBracket expected");
                    --it;
                    if (it->exprType == LineTokenizer::ExpressionToken::etName) { // this is a function call
                        actAtom.reset(etFunction);
                        if      (it->name == ".INV"     || it->name == "INV")   actAtom.functionType = futInv;
                        else if (it->name == ".SQRT"    || it->name == "SQRT")  actAtom.functionType = futSqrt;
                        else if (it->name == ".EXP"     || it->name == "EXP")   actAtom.functionType = futExp;
                        else if (it->name == ".LN"      || it->name == "LN")    actAtom.functionType = futLn;
                        else if (it->name == ".LOG"     || it->name == "LOG")   actAtom.functionType = futLog;
                        else if (it->name == ".ABS"     || it->name == "ABS")   actAtom.functionType = futAbs;
                        else if (it->name == ".QCNL"    || it->name == "QCNL")  actAtom.functionType = futQcnl;
                        else if (it->name == ".U"       || it->name == "U")     actAtom.functionType = futUnit;
                        else if (it->name == ".URAMP"   || it->name == "URAMP") actAtom.functionType = futUramp;
                        else if (it->name == ".SIN"     || it->name == "SIN")   actAtom.functionType = futSin;
                        else if (it->name == ".COS"     || it->name == "COS")   actAtom.functionType = futCos;
                        else if (it->name == ".TAN"     || it->name == "TAN")   actAtom.functionType = futTan;
                        else if (it->name == ".SINH"    || it->name == "SINH")  actAtom.functionType = futSinh;
                        else if (it->name == ".COSH"    || it->name == "COSH")  actAtom.functionType = futCosh;
                        else if (it->name == ".TANH"    || it->name == "TANH")  actAtom.functionType = futTanh;
                        else if (it->name == ".ASIN"    || it->name == "ASIN")  actAtom.functionType = futAsin;
                        else if (it->name == ".ACOS"    || it->name == "ACOS")  actAtom.functionType = futAcos;
                        else if (it->name == ".ATAN"    || it->name == "ATAN")  actAtom.functionType = futAtan;
                        else if (it->name == ".ASINH"   || it->name == "ASINH") actAtom.functionType = futAsinh;
                        else if (it->name == ".ACOSH"   || it->name == "ACOSH") actAtom.functionType = futAcosh;
                        else if (it->name == ".ATANH"   || it->name == "ATANH") actAtom.functionType = futAtanh;
                        else {
                            errorMessage = std::string("invalid function name: ") + it->name;
                            return false;
                        }
                        actAtom.isBool = false;
                        const SpiceExpressionAtom& opat = theExpression[start->storedTokenIndex];
                        if (actAtom.functionType == futUnit || actAtom.functionType == futUramp) {
                            actAtom.isConst = false;
                        }
                        else {
                            actAtom.isConst = opat.isConst;
                            if (actAtom.isConst) {
                                switch (actAtom.functionType) {
                                    case futInv:
                                        if (opat.constValue == 0) {
                                            errorMessage = "division by zero";
                                            return false;
                                        }
                                        actAtom.constValue = 1 / opat.constValue;   
                                        break;
                                    case futSqrt:   actAtom.constValue = sqrt(opat.constValue); break;
                                    case futExp:    actAtom.constValue = exp(opat.constValue);  break;
                                    case futLn:     actAtom.constValue = log(opat.constValue);  break;
                                    case futLog:    actAtom.constValue = log10(opat.constValue);break;
                                    case futAbs:    actAtom.constValue = fabs(opat.constValue); break;
                                    case futAsin:   actAtom.constValue = asin(opat.constValue); break;
                                    case futAcos:   actAtom.constValue = acos(opat.constValue); break;
                                    case futAtan:   actAtom.constValue = atan(opat.constValue); break;
                                    case futAsinh:  actAtom.constValue = asinh(opat.constValue);break;
                                    case futAcosh:  actAtom.constValue = acosh(opat.constValue);break;
                                    case futAtanh:  actAtom.constValue = atanh(opat.constValue);break;
                                    case futSin:    actAtom.constValue = sin(opat.constValue);  break;
                                    case futCos:    actAtom.constValue = cos(opat.constValue);  break;
                                    case futTan:    actAtom.constValue = tan(opat.constValue);  break;
                                    case futSinh:   actAtom.constValue = sinh(opat.constValue); break;
                                    case futCosh:   actAtom.constValue = cosh(opat.constValue); break;
                                    case futTanh:   actAtom.constValue = tanh(opat.constValue); break;
                                    case futQcnl:   actAtom.constValue = opat.constValue;       break;
                                    default:
                                        throw hmgExcept("SpiceExpression::buildFromString", "unknown const function");
                                }
                            }
                        }
                        actAtom.par1OrSourceIndex = start->storedTokenIndex;
                        start->storedTokenIndex = addToTheExpression(actAtom);
                        it = tokenList.erase(it); // del function name
                        ++it;
                        if (it != start)
                            throw hmgExcept("SpiceExpression::buildFromString", "unexpected list behaviour");
                    }
                    else { // an expression in brackets => remove brackets
                    }
                    it = start;
                    --it;
                    if (it->exprType != LineTokenizer::ExpressionToken::etOperator || it->opType != LineTokenizer::ExpressionToken::otOpenBracket)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otOpenBracket expected");
                    it = tokenList.erase(it);
                    ++it;
                    if (it != stop)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otCloseBracket expected");
                    tokenList.erase(it);
                    stop = it = start;
                }
            }
            else if (commaCount == 1 && start != tokenList.begin()) {
                // no two parameter function at the moment
                errorMessage = std::string("there is no two-parameter function");
                return false;
            }
            else { // create parameter list
                // .ratio, .pwl, 
                unsigned short prevIndex = 0;
                actAtom.reset(etListElem);
                it = start;
                for (unsigned i = 0; i < commaCount; i++) {
                    actAtom.par1OrSourceIndex = it->storedTokenIndex;
                    actAtom.par2OrPrevIndex = prevIndex;
                    prevIndex = addToTheExpression(actAtom);
                    it = tokenList.erase(it); // del token
                    it = tokenList.erase(it); // del comma
                }
                actAtom.par1OrSourceIndex = it->storedTokenIndex;
                actAtom.par2OrPrevIndex = prevIndex;
                it->storedTokenIndex = addToTheExpression(actAtom);
                start = it;
                if (start == tokenList.begin()) {
                    if (tokenList.erase(start) != tokenList.end())
                        throw hmgExcept("SpiceExpression::buildFromString", "Unexpected tokenList.erase(start)!=tokenList.end()");
                }
                else { // this is a function
                    it = start;
                    --it;
                    if (it->exprType != LineTokenizer::ExpressionToken::etOperator || it->opType != LineTokenizer::ExpressionToken::otOpenBracket)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otOpenBracket expected");
                    --it;
                    if (it->exprType != LineTokenizer::ExpressionToken::etName) { // comma separated expressions cannot be in parenthesis
                        errorMessage = std::string("comma separated expressions in bracket");
                        return false;
                    }
                    else { // this is a function call
                        actAtom.reset(etFunction);
                        if      (it->name == ".RATIO"   || it->name == "RATIO") actAtom.functionType = futRatio;
                        else if (it->name == ".PWL"     || it->name == "PWL")   actAtom.functionType = futPwl;
                        else {
                            errorMessage = std::string("invalid function name: ") + it->name;
                            return false;
                        }
                        switch (actAtom.functionType) {
                        case futRatio: 
                            if(commaCount!=2){
                                errorMessage = ".ratio must have 3 parameters";
                                return false;
                            }
                            break;
                        case futPwl:
                            if (commaCount % 2 != 0) {
                                errorMessage = ".pwl must have an odd number of parameters";
                                return false;
                            }
                            break;
                        }
                        actAtom.isBool = false;
                        actAtom.isConst = false;
                        actAtom.par1OrSourceIndex = start->storedTokenIndex;
                        start->storedTokenIndex = addToTheExpression(actAtom);
                        it = tokenList.erase(it); // del function name
                        ++it;
                        if (it != start)
                            throw hmgExcept("SpiceExpression::buildFromString", "unexpected list behaviour");
                    }
                    it = start;
                    --it;
                    if (it->exprType != LineTokenizer::ExpressionToken::etOperator || it->opType != LineTokenizer::ExpressionToken::otOpenBracket)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otOpenBracket expected");
                    it = tokenList.erase(it);
                    ++it;
                    if (it != stop)
                        throw hmgExcept("SpiceExpression::buildFromString", "LineTokenizer::ExpressionToken::otCloseBracket expected");
                    tokenList.erase(it);
                    stop = it = start;
                }
            }

        }
        else {
            // can be empty brackets: (), possible function call without parameter
            // however, at the moment there is no Hex Open function without parameter
            // in an operation the empty bracket pair is a syntax error
            errorMessage = "empty brackets: ()";
            return false;
        }
    }

    // optimization of constant expression parts
    
    for (unsigned i = 1; i < theExpression.size(); i++) {
        if (theExpression[i].atomType == etFunction && theExpression[i].isConst) {
            theExpression[i].atomType = etConst;
            theExpression[i].par1OrSourceIndex = theExpression[i].par2OrPrevIndex = 0;
            theExpression[i].functionType = futInvalid;
        }
    }
    for (unsigned i = 1; i < theExpression.size(); i++) {
        if (theExpression[i].isConst) {
            bool isUsed = false;
            for (unsigned j = i + 1; j < theExpression.size(); j++) {
                if (theExpression[j].par1OrSourceIndex == i || theExpression[j].par2OrPrevIndex == i) {
                    isUsed = true;
                    break;
                }
            }
            if (!isUsed) {
                for (unsigned j = i + 1; j < theExpression.size(); j++) {
                    if (theExpression[j].par1OrSourceIndex > i) theExpression[j].par1OrSourceIndex--;
                    if (theExpression[j].par2OrPrevIndex > i)   theExpression[j].par2OrPrevIndex--;
                }
                theExpression.erase(theExpression.begin() + i);
                i--;
            }
        }
    }

    return true;
}

}
