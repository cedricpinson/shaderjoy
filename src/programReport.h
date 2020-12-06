#pragma once

#include <functional>
#include <vector>

struct Line {
    const char* text = nullptr;
    size_t size = 0;
    size_t lineNumber = 0;
};

using LineList = std::vector<Line>;

// this enum is used to identified what type of line are dealing with when reporting errors
enum LineType {
    LINE_ERROR,        // line error reported by glGetShaderInfoLog
    LINE_SHADER_ERROR, // line of shader where the error occured
    LINE_SHADER        // regular shader line
};

using PrintLine = std::function<size_t(const Line& line, LineType lineType, int indentationError, char* buffer)>;

size_t printConsole(const Line& line, LineType lineType, int indentationError, char* buffer);

struct ShaderCompileReport {
    LineList shaderLines;
    LineList errorLines;
    std::vector<char> errorBuffer;
    std::vector<char> shaderBuffer;
    bool compileSuccess = false;
};

void createShaderReport(const char* shaderText,     // NOLINT
                        const char* errorLog,       // NOLINT
                        const char* preShaderText,  // NOLINT
                        const char* postShaderText, // NOLINT
                        ShaderCompileReport* shaderReport);

size_t generateShaderTextErrors(const ShaderCompileReport& shaderReport, char* buffer);
size_t generateShaderTextWithErrorsInlined(const ShaderCompileReport& shaderReport, char* buffer, PrintLine printLine);
int getShaderLineIndentation(const Line& line);
