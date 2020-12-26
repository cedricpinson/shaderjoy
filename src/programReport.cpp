#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "programReport.h"

// extract for the current type of errors
// ERROR: 0:18: Use of undeclared identifier 'color0'
// 0(18) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
// 0(23) : error C1503: undefined variable "offset"
bool extractLineError(Line& line, const char* errorLine, size_t errorLineSize)
{
    char tmp[16];
    if (memcmp("ERROR: 0:", errorLine, 9) == 0) {
        // parse this format:
        // ERROR: 0:18: Use of undeclared identifier 'color0'
        // ERROR: 0:18: Use of undeclared identifier 'color1'

        const char* lineNumberPtr = errorLine + 9;
        const char* endLineNumberPtr = strchr(lineNumberPtr, ':');
        assert(endLineNumberPtr != nullptr && "format of error not recognized: ERROR: 0:18: .....");

        const size_t lineNumberSize = size_t(endLineNumberPtr - lineNumberPtr);
        strncpy(tmp, lineNumberPtr, lineNumberSize);
        tmp[lineNumberSize] = 0;
        const int lineNumber = atoi(tmp);
        line.lineNumber = size_t(lineNumber);
        line.text = endLineNumberPtr + 2;
        line.size = size_t((errorLine + errorLineSize) - line.text);
        return true;
    }

    if (memcmp("0(", errorLine, 2) == 0) {
        // parse this format:
        // 0(18) : error C0000: syntax error, unexpected ';', expecting "::" at token ";"
        // 0(23) : error C1503: undefined variable "offset"

        const char* lineNumberPtr = errorLine + 2;
        const char* endLineNumberPtr = strchr(lineNumberPtr, ')');
        assert(endLineNumberPtr != nullptr && "0(23) : error ...");

        const size_t lineNumberSize = size_t(endLineNumberPtr - lineNumberPtr);
        strncpy(tmp, lineNumberPtr, lineNumberSize);
        tmp[lineNumberSize] = 0;
        const int lineNumber = atoi(tmp);
        line.lineNumber = size_t(lineNumber);
        line.text = strchr(endLineNumberPtr, ':') + 2;
        line.size = size_t((errorLine + errorLineSize) - line.text);
        return true;
    }

    return false;
}

void convertTextToLineList(const char* text, std::vector<Line>& lineList)
{
    const size_t textSize = strlen(text);
    const char* currentPointer = text;
    const char* endOfText = text + textSize;
    size_t lineCount = 0;
    while (true) {
        Line line;
        line.text = currentPointer;
        line.lineNumber = lineCount + 1;
        const char* nextLine = strchr(currentPointer, '\n');
        if (nextLine == nullptr) {
            line.size = size_t(endOfText - currentPointer);
            lineList.push_back(line);
            break;
        }
        line.size = size_t(nextLine - currentPointer);
        // printf("size %d\n", line.size);
        lineList.push_back(line);

        lineCount++;
        currentPointer = nextLine + 1;
    }
}

void convertErrorTextToLineList(const char* text, std::vector<Line>& errorList)
{
    std::vector<Line> lineList;
    convertTextToLineList(text, lineList);

    for (auto&& line : lineList) {
        Line error;
        if (extractLineError(error, line.text, line.size)) {
            errorList.emplace_back(error);
        }
    }
}

size_t lineCount(const char* text)
{
    const char* currentPointer = text;
    size_t lineCount = 0;
    while (true) {
        const char* nextLine = strchr(currentPointer, '\n');
        if (nextLine == nullptr) {
            break;
        }
        lineCount++;
        currentPointer = nextLine + 1;
    }
    return lineCount;
}

int getShaderLineIndentation(const Line& line)
{
    int i = 0;
    // count character that are differnt from space
    while (i < static_cast<int>(line.size)) {
        if (line.text[i] != ' ')
            return i;
        i++;
    }
    return 0;
}

size_t printConsole(const Line& line, LineType lineType, int indentationError, char* buffer)
{
    switch (lineType) {
    case LINE_ERROR:
        return (size_t)sprintf(buffer, "\033[33;3m %*s%.*s\033[0m\n", indentationError + 5, "", int(line.size),
                               line.text);
    case LINE_SHADER_ERROR:
        return (size_t)sprintf(buffer, "\033[31;1m %3d :%.*s\033[0m\n", int(line.lineNumber), int(line.size),
                               line.text);
    case LINE_SHADER:
        return (size_t)sprintf(buffer, " %3d :%.*s\n", int(line.lineNumber), int(line.size), line.text);
    }
    return 0;
}

size_t generateShaderTextWithErrorsInlined(const ShaderCompileReport& shaderReport, char* buffer,
                                           const PrintLine& printLine)
{
    const std::vector<Line>& shaderLines = shaderReport.shaderLines;
    const std::vector<Line>& errorLines = shaderReport.errorLines;

    size_t errorIndex = 0;
    size_t resultIndex = 0;
    for (size_t i = 0; i < shaderLines.size(); ++i) {
        const size_t lineNumder = shaderLines[i].lineNumber;
        bool hasError = false;
        int indentation = 0;
        for (size_t j = errorIndex; j < errorLines.size(); j++) {
            if (errorLines[j].lineNumber == lineNumder) {
                hasError = true;
                if (!indentation) {
                    indentation = getShaderLineIndentation(shaderLines[i]);
                }
                resultIndex += printLine(errorLines[j], LINE_ERROR, indentation, buffer + resultIndex);
            }
        }

        if (hasError) {
            resultIndex += printLine(shaderLines[i], LINE_SHADER_ERROR, 0, buffer + resultIndex);
        } else {
            resultIndex += printLine(shaderLines[i], LINE_SHADER, 0, buffer + resultIndex);
        }
    }
    if (buffer) {
        buffer[resultIndex] = 0;
    }
    return resultIndex;
}

size_t generateShaderTextErrors(const ShaderCompileReport& shaderReport, char* buffer)
{
    size_t resultErrorIndex = 0;
    for (auto&& line : shaderReport.errorLines) {
        resultErrorIndex +=
            (size_t)sprintf(buffer + resultErrorIndex, " %3d :%.*s\n", int(line.lineNumber), int(line.size), line.text);
    }
    buffer[resultErrorIndex] = 0;
    return resultErrorIndex;
}

void createShaderReport(const char* shaderText,     // NOLINT
                        const char* errorLog,       // NOLINT
                        const char* preShaderText,  // NOLINT
                        const char* postShaderText, // NOLINT
                        ShaderCompileReport* shaderReport)
{
    std::vector<Line>& errors = shaderReport->errorLines;
    errors.reserve(100);
    errors.clear();

    std::vector<Line>& shader = shaderReport->shaderLines;
    shader.reserve(1000);
    shader.clear();

    if (errorLog) {
        convertErrorTextToLineList(errorLog, errors);
    }

    convertTextToLineList(shaderText, shader);

    const size_t preShaderLineCount = lineCount(preShaderText);
    const size_t postShaderLineCount = lineCount(postShaderText);

    // strip postShaderText lines
    shader.erase(shader.begin() + int(shader.size() - postShaderLineCount), shader.end());
    // strip preShaderText lines
    shader.erase(shader.begin(), shader.begin() + int(preShaderLineCount));
    // std::vector<Line> shaderClean(&shader[preShaderLineCount], &shader[shader.size() - postShaderLineCount]);

    // then substrace line numbers
    for (auto&& line : shader) {
        line.lineNumber -= preShaderLineCount;
    }

    // then substrace line numbers
    for (auto&& line : errors) {
        line.lineNumber -= preShaderLineCount;
    }
}
