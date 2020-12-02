#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// try to get error lines numbers
// ERROR: 0:18: Use of undeclared identifier 'color0'
// ERROR: 0:18: Use of undeclared identifier 'color1'
// 0(18) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
// 0(23) : error C1503: undefined variable "offset"
struct LineError {
    const char* error = nullptr;
    int lineNumber = 0;
    int errorSize = 0;
};

bool extractLineError(LineError& lineError, const char* nextReturn, const char* nlineCharacter,
                      const char* nextDelimiter, const int textLineCount)
{
    const size_t lineNumberSize = nextDelimiter - nlineCharacter;
    if (!nextDelimiter || lineNumberSize > 8) {
        return false;
    }
    char tmp[8]{};
    strncpy(tmp, nlineCharacter, lineNumberSize);
    const int lineNumber = atoi(tmp);
    // be sure the value makes sense if it's out of range we probably get a bad number
    if (lineNumber < 0 || lineNumber >= textLineCount)
        return false;
    lineError.lineNumber = lineNumber;
    lineError.error = nextDelimiter + 2;
    lineError.errorSize = int(nextReturn - lineError.error);
    return true;
}

int getLineCount(const char* text)
{
    const char* currentPointer = text;
    int lineCount = 0;
    while (true) {
        const char* nextLine = strchr(currentPointer, '\n');
        if (nextLine == nullptr)
            break;

        lineCount++;
        currentPointer = nextLine + 1;
    }
    return lineCount;
}

int getLineErrors(LineError* lineError, const char* text, const int shaderLineCount)
{
    const char* currentPointer = text;
    const int size = int(strlen(text));
    int nbLines = 0;
    while (currentPointer < &text[size]) {

        const char* nextCharacter0 = strchr(currentPointer, '0');
        const char* nextReturn = strchr(currentPointer, '\n');

        if (!nextCharacter0 || !nextReturn) {
            break;
        }

        const size_t indexCharacter0 = nextCharacter0 - currentPointer;
        const size_t lineSize = nextReturn - currentPointer;

        // candidate to extract line number
        if (indexCharacter0 == 7 && (nextCharacter0[1] == ':') && lineSize > 10) {
            // ERROR: 0:18: Use of undeclared identifier 'color0'
            //        ^
            const char* lineNumberCharacter = nextCharacter0 + 2;
            const char* nextDelimiter = strchr(lineNumberCharacter, ':');
            const bool validLineNumber =
                extractLineError(lineError[nbLines], nextReturn, lineNumberCharacter, nextDelimiter, shaderLineCount);
            if (validLineNumber)
                nbLines++;

        } else if (indexCharacter0 == 0 && (nextCharacter0[1] == '(') && lineSize > 3) {
            // 0(18) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
            // ^
            const char* lineNumberCharacter = nextCharacter0 + 2;
            const char* nextDelimiter = strchr(lineNumberCharacter, ')');
            const bool validLineNumber =
                extractLineError(lineError[nbLines], nextReturn, lineNumberCharacter, nextDelimiter, shaderLineCount);
            if (validLineNumber)
                nbLines++;
        }

        currentPointer = nextReturn + 1;
    }
    return nbLines;
}

int findIndentation(const char* line)
{
    int i = 0;
    // count character that are differnt from space
    while (line[i] != '\0') {
        if (line[i] != ' ')
            return i;
        i++;
    }
    return 0;
}

bool injectErrorInline(const LineError* lineError, const int lineErrorCount, const int noLine, const char* line)
{
    bool hasError = false;
    int nbCharacterIndentation = 0;
    bool computedIndentation = false;
    for (int i = 0; i < lineErrorCount; ++i) {
        if (lineError[i].lineNumber == noLine) {
            if (!computedIndentation) {
                nbCharacterIndentation = findIndentation(line);
                computedIndentation = true;
            }
            fprintf(stderr, "\033[33;3m %*s%.*s\033[0m\n", nbCharacterIndentation + 5, "", lineError[i].errorSize,
                    lineError[i].error);
            hasError = true;
        }
    }
    return hasError;
}

void debugShader(const char* text, const char* error)
{
    LineError lineErrors[1024];
    const char* currentPointer = text;
    const int size = int(strlen(text));
    const int textLineCount = getLineCount(text);
    const int lineErrorCount = getLineErrors(lineErrors, error, textLineCount);
    int nLine = 1;
    char line[1024]{};

    while (currentPointer < &text[size]) {
        const char* nextReturn = strchr(currentPointer, '\n');
        if (!nextReturn) {
            break;
        }
        const int lineSize = int(nextReturn - currentPointer);
        strncpy(line, currentPointer, lineSize);
        line[lineSize] = '\0';
        const bool hasError = injectErrorInline(lineErrors, lineErrorCount, nLine, line);
        if (hasError) {
            fprintf(stderr, "\033[31;1m %3d :%s\033[0m\n", nLine, line);
        } else {
            fprintf(stderr, " %3d :%s\n", nLine, line);
        }
        nLine++;
        currentPointer = nextReturn + 1;
    }
}
