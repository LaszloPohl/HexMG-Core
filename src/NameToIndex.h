//***********************************************************************
// Hex Open NameToIndex Header
// Creation date:  2021. 06. 23.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HO_NAMETOINDEX_HEADER
#define HO_NAMETOINDEX_HEADER
//***********************************************************************


//***********************************************************************
#include <vector>
#include "hmgException.h"
#include "hmgCommon.h"
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
    inline static const char* const specCharsOfNames = "_.:@$";
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
    const char* getNextToken(const char* fileName, unsigned lineNumber) {
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
    struct ExpressionToken {
    //***********************************************************************
        enum OperatorType {
            otNone, otOpenBracket, otCloseBracket, otComma, otPlus, otMinus, otMul, otDiv, otPow,
            otGreater, otSmaller, otGrEq, otSmEq, otEqual, otNonEqual, otAnd, otOr, otNot,
            otMinusSign
        };
        enum ExprType {
            etError, etName, etNumber, etOperator, etStoredToken
        };
        ExprType exprType;
        OperatorType opType;
        unsigned short storedTokenIndex;
        std::string name;
        double value;
        ExpressionToken() :exprType{ etError }, opType{ otNone }, storedTokenIndex{ 0 }, value{ 0 }{}
    };

    //***********************************************************************
    ExpressionToken getNextExpressionToken() {
    //***********************************************************************
        while (pLine[position] == ' ')
            position++;
        ExpressionToken ret;
        if (errorMessage.length() != 0)errorMessage.clear();

        if (isSpecialExpressionToken) {
            if (specExprToken == settVFirst) {
                ret.exprType = ExpressionToken::etName;
                skipSeparators();
                unsigned tokenpos = 0;
                if (pLine[position] != '@')
                    token[tokenpos++] = '@';
                while (isalnum(pLine[position]) || isSpecChar(pLine[position]))
                    token[tokenpos++] = pLine[position++];
                token[tokenpos] = 0;
                while (pLine[position] != ',')
                    position++;
                ret.name = token;
                setIsSeparators();
                specExprToken = settVComma;
                return ret;
            }
            if (specExprToken == settVComma) {
                if (pLine[position] != ',')
                    throw hmgExcept("LineTokenizer::getNextExpressionToken", "pLine[position] != ',' (%c)", pLine[position]);
                ret.exprType = ExpressionToken::etOperator;
                ret.opType = ExpressionToken::otMinus;
                position++;
                setIsSeparators();
                specExprToken = settVSecond;
                return ret;
            }
            if (specExprToken == settVSecond) {
                ret.exprType = ExpressionToken::etName;
                skipSeparators();
                unsigned tokenpos = 0;
                if (pLine[position] != '@')
                    token[tokenpos++] = '@';
                while (isalnum(pLine[position]) || isSpecChar(pLine[position]))
                    token[tokenpos++] = pLine[position++];
                token[tokenpos] = 0;
                setIsSeparators();
                if (isSepClosingBracket) {
                    while (pLine[position] != ')')// position to the ')'
                        position++;
                    ret.name = token;
                    setIsSeparators();
                    specExprToken = settVClose;
                    return ret;
                }
                ret.exprType = ExpressionToken::etError; // ')' should come
                errorMessage = "\')\' missing";
                return ret;
            }
            if (specExprToken == settVClose) {
                if (pLine[position] != ')')
                    throw hmgExcept("LineTokenizer::getNextExpressionToken", "pLine[position] != ')' (%c)", pLine[position]);
                ret.exprType = ExpressionToken::etOperator;
                ret.opType = ExpressionToken::otCloseBracket;
                position++;
                setIsSeparators();
                isSpecialExpressionToken = false;
                specExprToken = setttNone;
                return ret;
            }
        }

        // operator

        ret.exprType = ExpressionToken::etOperator;
        switch (pLine[position]) {
            case '(': ret.opType = ExpressionToken::otOpenBracket; position++; setIsSeparators(); return ret;
            case ')': ret.opType = ExpressionToken::otCloseBracket; position++; setIsSeparators(); return ret;
            case ',': ret.opType = ExpressionToken::otComma; position++; setIsSeparators(); return ret;
            case '+': ret.opType = ExpressionToken::otPlus; position++; setIsSeparators(); return ret;
            case '-': ret.opType = ExpressionToken::otMinus; position++; setIsSeparators(); return ret;
            case '*': ret.opType = ExpressionToken::otMul; position++; setIsSeparators(); return ret;
            case '/': ret.opType = ExpressionToken::otDiv; position++; setIsSeparators(); return ret;
            case '^': ret.opType = ExpressionToken::otPow; position++; setIsSeparators(); return ret;
            case '>': 
                if (pLine[position + 1] == '=') { 
                    ret.opType = ExpressionToken::otGrEq; 
                    position += 2;
                }
                else {
                    ret.opType = ExpressionToken::otGreater;
                    position++;
                }
                setIsSeparators();
                return ret;
            case '<':
                if (pLine[position + 1] == '=') {
                    ret.opType = ExpressionToken::otSmEq;
                    position += 2;
                }
                else {
                    ret.opType = ExpressionToken::otSmaller;
                    position++;
                }
                setIsSeparators();
                return ret;
            case '=':
                if (pLine[position + 1] == '=') {
                    position += 2;
                }
                else {
                    ret.exprType = ExpressionToken::etError;
                    errorMessage = "\'=\' found instead of \'==\'";
                    position++;
                }
                ret.opType = ExpressionToken::otEqual;
                setIsSeparators();
                return ret;
            case '!':
                if (pLine[position + 1] == '=') {
                    ret.opType = ExpressionToken::otNonEqual;
                    position += 2;
                }
                else {
                    ret.opType = ExpressionToken::otNot;
                    position++;
                }
                setIsSeparators();
                return ret;
            case '&':
                if (pLine[position + 1] == '&') position += 2;
                else position++;
                ret.opType = ExpressionToken::otAnd;
                setIsSeparators();
                return ret;
            case '|':
                if (pLine[position + 1] == '|') position += 2;
                else position++;
                ret.opType = ExpressionToken::otOr;
                setIsSeparators();
                return ret;
        }

        // number

        if (isdigit(pLine[position]) || (pLine[position] == '.' && isdigit(pLine[position + 1]))) {
            ret.exprType = ExpressionToken::etNumber;
            double baseNumber = 0;
            if (sscanf_s(pLine + position, "%lg", &baseNumber) != 1) {
                ret.exprType = ExpressionToken::etError;
                setIsSeparators();
                errorMessage = "failed reading number";
                return ret;
            }
            while (pLine[position] == '.' || isdigit(pLine[position]))
                position++;
            if (pLine[position] == 'E') {
                position++;
                if (pLine[position] == '+' || pLine[position] == '-') {
                    position++;
                }
                if (!isdigit(pLine[position])) {
                    ret.exprType = ExpressionToken::etError;
                    setIsSeparators();
                    errorMessage = "unknown number format after e";
                    return ret;
                }
                while (isdigit(pLine[position]))
                    position++;
            }
            if (isalpha(pLine[position])) {
                switch (pLine[position]) {
                    case 'T': baseNumber *= 1e12; break;
                    case 'G': baseNumber *= 1e9; break;
                    case 'M': 
                        if (pLine[position + 1] == 'E' && pLine[position + 2] == 'G')
                            baseNumber *= 1e6;
                        else if (pLine[position + 1] == 'I' && pLine[position + 2] == 'L')
                            baseNumber *= 25.4e-6;
                        else baseNumber *= 0.001;
                        break;
                    case 'K': baseNumber *= 1000; break;
                    case 'U': baseNumber *= 1e-6; break;
                    case 'N': baseNumber *= 1e-9; break;
                    case 'P': baseNumber *= 1e-12; break;
                    case 'F': baseNumber *= 1e-15; break;
                }
                while (isalpha(pLine[position]))
                    position++;
            }
            ret.value = baseNumber;
            setIsSeparators();
            return ret;
        }

        // name

        if (isalnum(pLine[position]) || isSpecChar(pLine[position])) {
            ret.exprType = ExpressionToken::etName;
            unsigned i = 0;
            for (; isalnum(pLine[position]) || isSpecChar(pLine[position]); i++, position++)
                token[i] = pLine[position];
            token[i] = 0;
            if (strcmp(token, "_PI") == 0) {
                ret.exprType = ExpressionToken::etNumber;
                ret.value = hmgPi;
                setIsSeparators();
                return ret;
            }
            if (strcmp(token, "_E") == 0) {
                ret.exprType = ExpressionToken::etNumber;
                ret.value = hmgE;
                setIsSeparators();
                return ret;
            }
            if (strcmp(token, "_K") == 0) {
                ret.exprType = ExpressionToken::etNumber;
                ret.value = hmgK;
                setIsSeparators();
                return ret;
            }
            if (strcmp(token, "_Q") == 0) {
                ret.exprType = ExpressionToken::etNumber;
                ret.value = hmgQ;
                setIsSeparators();
                return ret;
            }
            if (strcmp(token, "_TA") == 0) {
                ret.exprType = ExpressionToken::etNumber;
                ret.value = hmgT0;
                setIsSeparators();
                return ret;
            }
            if (strcmp(token, "V") == 0) {
                setIsSeparators();
                if (isSepOpeningBracket) { //v(N1) or v(N1,N2) where N1 or N2 can be just a number without @
                    unsigned startingPos = position;
                    skipSeparators();
                    unsigned tokenpos = 0;
                    if (pLine[position] != '@')
                        token[tokenpos++] = '@';
                    while (isalnum(pLine[position]) || isSpecChar(pLine[position]))
                        token[tokenpos++] = pLine[position++];
                    token[tokenpos] = 0;
                    setIsSeparators();
                    if (isSepClosingBracket) { // v(N1)
                        while (pLine[position++] != ')')// position after the ')'
                            ;
                        ret.name = token;
                        setIsSeparators();
                        return ret;
                    }
                    if (!isSepComma) { // v(token?????
                        ret.exprType = ExpressionToken::etError;
                        errorMessage = ",')\' missing";
                        return ret;
                    }
                    // v(N1,N2)
                    position = startingPos;
                    ret.exprType = ExpressionToken::etOperator;
                    ret.opType = ExpressionToken::otOpenBracket;
                    while (pLine[position++] != '(')
                        ;
                    isSpecialExpressionToken = true;
                    specExprToken = settVFirst;
                    setIsSeparators(); 
                    return ret;
                }
            }
            if (strcmp(token, "I") == 0) {
                setIsSeparators();
                if (isSepOpeningBracket) { // I(comp1) where comp1 can be just a number without @
                    skipSeparators();
                    unsigned tokenpos = 0;
                    if (pLine[position] != '@')
                        token[tokenpos++] = '@';
                    while (isalnum(pLine[position]) || isSpecChar(pLine[position]))
                        token[tokenpos++] = pLine[position++];
                    token[tokenpos++] = '.';
                    token[tokenpos++] = 'I';
                    token[tokenpos] = 0;
                    setIsSeparators();
                    if (isSepClosingBracket) { 
                        while (pLine[position++] != ')')// position after the ')'
                            ;
                        ret.name = token;
                        setIsSeparators();
                        return ret;
                    }
                    ret.exprType = ExpressionToken::etError;
                    errorMessage = "\')\' missing";
                    return ret;
                }
            }
            ret.name = token;
            setIsSeparators();
            return ret;
        }

        ret.exprType = ExpressionToken::etError;
        setIsSeparators();
        errorMessage = "invalid text format";
        return ret;
    }
};


class InstructionStream;


//***********************************************************************
struct SpiceExpression {
//***********************************************************************
    //***********************************************************************
    struct SpiceExpressionAtom: public ExpressionAtom {
    //***********************************************************************
        std::string name;
        SpiceExpressionAtom(ExpressionAndComponentType type = etInvalid) : ExpressionAtom{ type }{}
        void reset(ExpressionAndComponentType type) { *this = SpiceExpressionAtom(type); }
    };
    //***********************************************************************
    std::vector<SpiceExpressionAtom> theExpression;
    std::string errorMessage;
    unsigned short addToTheExpression(const SpiceExpressionAtom& actAtom);
    bool buildFromString(const char* spiceExpression);
    void toInstructionStream(InstructionStream& iStream, unsigned index);
    //***********************************************************************
};


}


#endif

