#include "Application.h"
#include "ProgramDescription.h"
#include "UniformList.h"
#include "glad/glad.h"
#include "screenShoot.h"
#include "timer.h"
#include "watcher.h"
#include "window.h"

#include <sys/stat.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

void dumpFileEntry(const WatchFile& fileEntry)
{
    if (fileEntry.type == WatchFile::SHADER) {
        printf("shader: %s\n", fileEntry.path.c_str());
    } else {
        switch (fileEntry.type) {
        case WatchFile::TEXTURE0:
            printf("texture0:");
            break;
        case WatchFile::TEXTURE1:
            printf("texture1:");
            break;
        case WatchFile::TEXTURE2:
            printf("texture2:");
            break;
        case WatchFile::TEXTURE3:
            printf("texture3:");
            break;
        default:
            break;
        }

        if (fileEntry.texture.target == Texture::TEXTURE_2D) {
            printf("2D:");
        } else {
            printf("3D:");
        }

        switch (fileEntry.texture.format) {
        case Texture::RGBA:
            printf("RGBA:");
            break;
        case Texture::RGB:
            printf("RGB:");
            break;
        case Texture::R:
            printf("R:");
            break;
        }

        switch (fileEntry.texture.filter) {
        case Texture::LINEAR:
            printf("linear:");
            break;
        case Texture::LINEAR_MIPMAP_LINEAR:
            printf("linear_mipmap_linear:");
            break;
        case Texture::NEAREST:
            printf("nearest:");
            break;
        }
        switch (fileEntry.texture.wrap) {
        case Texture::REPEAT:
            printf("repeat:");
            break;
        case Texture::CLAMP:
            printf("clamp:");
            break;
        }

        printf(" %s\n", fileEntry.path.c_str());
    }
}

float clamp(const float v, const float min, const float max)
{
    if (v < min)
        return min;
    if (v > max)
        return max;
    return v;
}

void initIMGUI(GLFWwindow* window);
void drawIMGUI();
void cleanupIMGUI();

void debugShader(const char* shaderText, const char* errorLog, const char* preShaderText, const char* postShaderText,
                 char* resultShaderWithErrors, size_t& resultShaderWithErrorsSize, char* resultErrors,
                 size_t& resultErrorsSize);

const char* defaultVertex = R"(
#version 330

in vec2 vp;

void main() {
  gl_Position = vec4(vp, 0.0, 1.0);
}

)";

const char* defaultFragment = R"(
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(col,1.0);
}
)";

const char* defaultTemplatePreFragment = nullptr;

const char* defaultTemplatePostFragment = R"(
void main() {

  vec4 color;
  mainImage(color, gl_FragCoord.xy);
  frag_colour = color;
}

)";

bool compileShader(const char* shaderText, GLenum shaderType, GLuint& shader,
                   ShaderCompileReport* shaderReport = nullptr)
{
    // printf("real shader:\n%s\n", shaderText);
    // buffer twice the shader size
    const size_t bufferSize = strlen(shaderText) * 2;
    std::vector<char> tmpBuffer(bufferSize + 1);

    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderText, NULL);
    glCompileShader(shader);
    GLint shaderResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderResult);

    // shaderReport is null for vertex shader because it's not supposed to fails
    if (shaderReport) {
        shaderReport->errorBuffer.resize(bufferSize);
    }

    if (!shaderResult) {
        GLsizei size;
        char* infoLog = shaderReport ? shaderReport->errorBuffer.data() : tmpBuffer.data();
        glGetShaderInfoLog(shader, bufferSize, &size, infoLog);
        infoLog[size] = 0;

        // in case we have a problem with vertex shader printf in console
        if (!shaderReport) {
            printf("fails to compile shader:\n%s", infoLog);
        } else {
            shaderReport->errorBuffer.resize(size_t(size));
        }
    }

    if (shaderReport) {
        createShaderReport(shaderText, shaderResult ? nullptr : shaderReport->errorBuffer.data(),
                           defaultTemplatePreFragment, defaultTemplatePostFragment, shaderReport);

        if (!shaderResult) {

            const size_t shaderTextSize =
                generateShaderTextWithErrorsInlined(*shaderReport, tmpBuffer.data(), printConsole);
            (void)shaderTextSize;
            assert(shaderTextSize < bufferSize && "BufferSize too small to report shader errors");
            printf("shader failed to compile:\n%s\n", tmpBuffer.data());

            const size_t errorTextSize = generateShaderTextErrors(*shaderReport, tmpBuffer.data());
            (void)errorTextSize;
            assert(errorTextSize < bufferSize && "BufferSize too small to report shader errors");
            printf("\nerrors lists:\n%s\n", tmpBuffer.data());
        } else {
            const size_t shaderTextSize =
                generateShaderTextWithErrorsInlined(*shaderReport, tmpBuffer.data(), printConsole);
            (void)shaderTextSize;
            assert(shaderTextSize < bufferSize && "BufferSize too small to display shader");
            printf("%s\n", tmpBuffer.data());
        }
        shaderReport->compileSuccess = shaderResult;
    }

    return shaderResult;
}

void outputError(int error, const char* msg) { fprintf(stderr, "Error%d: %s\n", error, msg); }

void getProgramDescription(const GLuint program, ProgramDescription& description)
{
    GLsizei length; // name length
    GLint size;     // size of the variable
    GLenum type;    // type of the variable (float, vec3 or mat4, etc)

    GLint nbActiveAttributes;
    char name[1024];
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &nbActiveAttributes);

    // printf("Attributes %d\n", nbActiveAttributes);
    for (int i = 0; i < nbActiveAttributes; i++) {
        glGetActiveAttrib(program, (GLuint)i, 32, &length, &size, &type, name);
        // printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
        int location = glGetAttribLocation(program, name);
        ProgramDescription::Attribute attribute;
        attribute.name = name;
        attribute.location = location;
        description.attributes.emplace_back(attribute);
    }

    GLint nbActiveUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nbActiveUniforms);
    // printf("Uniforms %d\n", nbActiveAttributes);
    for (int i = 0; i < nbActiveUniforms; i++) {
        GLint uniformSize;
        GLenum uniformType;
        glGetActiveUniform(program, (GLuint)i, 32, &length, &uniformSize, &uniformType, name);

        ProgramDescription::Uniform uniform;
        uniform.size = uniformSize;
        uniform.type = uniformType;
        uniform.location = glGetUniformLocation(program, name);
        uniform.name = name;

        // printf("Uniform #%d Type: %u Name: %s\n", i, uniformType, name);
        description.uniforms.emplace_back(uniform);
    }
}

int getUniformLocation(const ProgramDescription* programDescription, const char* name)
{
    const std::string uniformName(name);
    for (const auto& uniform : programDescription->uniforms) {
        if (uniformName == uniform.name)
            return uniform.location;
    }
    return -1;
}

int getUniformIndex(const ProgramDescription* programDescription, const char* name)
{
    const std::string uniformName(name);
    for (size_t i = 0; i < programDescription->uniforms.size(); ++i) {
        if (uniformName == programDescription->uniforms[i].name && programDescription->uniforms[i].location != -1)
            return int(i);
    }
    return -1;
}

bool compileProgram(const GLuint vs, const char* shader, const size_t size, GLuint& fs, GLuint& program,
                    ShaderCompileReport& shaderReport)
{
    std::vector<char>& fullShader = shaderReport.shaderBuffer;
    fullShader.resize(size + 1024);
    size_t index = 0;
    size_t fragmentSize = 0;
    fragmentSize = strlen(defaultTemplatePreFragment);
    memcpy(fullShader.data() + index, defaultTemplatePreFragment, fragmentSize);
    index += fragmentSize;
    memcpy(fullShader.data() + index, shader, size);
    index += size;
    fragmentSize = strlen(defaultTemplatePostFragment);
    memcpy(fullShader.data() + index, defaultTemplatePostFragment, fragmentSize);
    index += fragmentSize;
    fullShader[index] = 0;
    fullShader.resize(index);

    GLuint newFS = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fullShader.data(), GL_FRAGMENT_SHADER, newFS, &shaderReport)) {
        glDeleteShader(newFS);
        return false;
    }

    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, newFS);
    glAttachShader(newProgram, vs);
    glLinkProgram(newProgram);
    GLint status;
    glGetProgramiv(newProgram, GL_LINK_STATUS, &status);
    if (!status) {
        glDeleteShader(newFS);
        glDeleteProgram(newProgram);
        return false;
    }

    // success and an older program exist, so delete it
    if (program != -1U) {
        glDeleteProgram(program);
        glDeleteShader(fs);
    }

    program = newProgram;
    fs = newFS;

    return true;
}

void updateTexture(GLuint& textureID, float* size, const Texture& texture)
{
    if (textureID != ~0x0u) {
        glDeleteTextures(1, &textureID);
    }

    GLint wrap = texture.wrap == Texture::REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    GLint minFilter;
    GLint magFilter;
    GLint format;
    GLenum type = GL_UNSIGNED_BYTE;

    switch (texture.format) {
    case Texture::RGB:
        format = GL_RGB;
        break;
    case Texture::RGBA:
        format = GL_RGBA;
        break;
    case Texture::R:
        format = GL_RED;
        break;
    }

    switch (texture.filter) {
    case Texture::LINEAR:
        magFilter = minFilter = GL_LINEAR;
        break;
    case Texture::LINEAR_MIPMAP_LINEAR:
        minFilter = GL_LINEAR_MIPMAP_LINEAR;
        magFilter = GL_LINEAR;
        break;
    case Texture::NEAREST:
        magFilter = minFilter = GL_NEAREST;
        break;
    }

    size[0] = texture.size[0];
    size[1] = texture.size[1];
    size[2] = texture.size[2];

    glGenTextures(1, &textureID);

    if (texture.target == Texture::TEXTURE_2D) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        // if ()
        glPixelStorei(GL_UNPACK_ALIGNMENT, texture.size[0] % 4 ? 1 : 4);
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.size[0], texture.size[1], 0, GLenum(format), type,
                     texture.data.data());

        if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

    } else {
        glBindTexture(GL_TEXTURE_3D, textureID);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, magFilter);

        glTexImage3D(GL_TEXTURE_3D, 0, format, texture.size[0], texture.size[1], texture.size[2], 0, GLenum(format),
                     type, texture.data.data());

        if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
            glGenerateMipmap(GL_TEXTURE_3D);
        }
    }
}

void getUniformList(const ProgramDescription* description, UniformList& uniforms)
{
    uniforms.iMouseLocation = getUniformLocation(description, "iMouse");
    uniforms.iTimeLocation = getUniformLocation(description, "iTime");
    uniforms.iResolutionLocation = getUniformLocation(description, "iResolution");
    uniforms.iTimeDeltaLocation = getUniformLocation(description, "iTimeDelta");
    uniforms.iFrameLocation = getUniformLocation(description, "iFrame");

    for (int i = 0; i < 4; i++) {
        char tmp[32];
        sprintf(tmp, "iChannel%d", i);
        uniforms.iChannelLocation[i] = getUniformLocation(description, tmp);
    }
    uniforms.iChannelResolutionLocation = getUniformLocation(description, "iChannelResolution");
}

//#define DISPLAY_UNIFORM

void updateUniforms(UniformList uniforms)
{
    // iChannels
    for (int i = 0; i < 4; i++) {
        if (uniforms.iChannelLocation[i] != -1) {
#ifdef DISPLAY_UNIFORM
            char tmp[32];
            sprintf(tmp, "iChannel%d", i);
            printf("%s %d: %d\n", tmp, uniforms.iChannelLocation[i], uniforms.iChannel[i]);
#endif
            glUniform1iv(uniforms.iChannelLocation[i], 1, &uniforms.iChannel[i]);
        }
    }
    if (uniforms.iChannelResolutionLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iChannelResolution %d: %f %f %f\n                    %f %f %f\n                    %f %f %f\n          "
               "          %f %f %f\n ",
               uniforms.iChannelResolutionLocation, uniforms.iChannelResolution[0][0],
               uniforms.iChannelResolution[0][1], uniforms.iChannelResolution[0][2], uniforms.iChannelResolution[1][0],
               uniforms.iChannelResolution[1][1], uniforms.iChannelResolution[1][2], uniforms.iChannelResolution[2][0],
               uniforms.iChannelResolution[2][1], uniforms.iChannelResolution[2][2], uniforms.iChannelResolution[3][0],
               uniforms.iChannelResolution[3][1], uniforms.iChannelResolution[3][2]);
#endif
        glUniform3fv(uniforms.iChannelResolutionLocation, 4, &uniforms.iChannelResolution[0][0]);
    }

    // vec4
    if (uniforms.iMouseLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iMouse %d: %f %f %f %f\n", uniforms.iMouseLocation, uniforms.iMouse[0], uniforms.iMouse[1],
               uniforms.iMouse[2], uniforms.iMouse[3]);
#endif
        glUniform4fv(uniforms.iMouseLocation, 1, uniforms.iMouse);
    }

    // vec3
    if (uniforms.iResolutionLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iResolution %d: %f %f %f\n", uniforms.iResolutionLocation, uniforms.iResolution[0],
               uniforms.iResolution[1], uniforms.iResolution[2]);
#endif
        glUniform3fv(uniforms.iResolutionLocation, 1, uniforms.iResolution);
    }

    // float
    if (uniforms.iTimeLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iTime %d: %f\n", uniforms.iTimeLocation, uniforms.iTime);
#endif
        glUniform1f(uniforms.iTimeLocation, uniforms.iTime);
    }

    if (uniforms.iTimeDeltaLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iTimeDelta %f\n", uniforms.iTimeDelta);
#endif
        glUniform1f(uniforms.iTimeDeltaLocation, uniforms.iTimeDelta);
    }

    // int
    if (uniforms.iFrameLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iFrame %d\n", uniforms.iFrame);
#endif
        glUniform1i(uniforms.iFrameLocation, uniforms.iFrame);
    }
}

void frameIMGUI(Application* app, const UniformList& uniformList);

void drawFrame() {}

void usage()
{
    printf("run shaderjoy with only one shader file\n");
    printf("shaderjoy [--save-frame] shader-file.glsl\n");
    printf("\nrun shaderjoy with texture:\n");
    printf("shaderjoy --texture0 [2d:linear:repeat] texture.png fragment.glsl\n");
    printf("shaderjoy --texture0 [3d:linear:repeat:sizex:sizey:sizez] texture.data fragment.glsl\n");
    printf("\nto report issue: https://github.com/cedricpinson/shaderjoy/issues\n");
}

bool parseTextureBlock(const char** argv, int i, Texture& texture)
{
    // --texture0 [2d:linear:repeat] filename
    // --texture0 [3d:linear:repeat:sizex:sizey:sizez] filename
    const char* endPtr = strchr(argv[i], ']');
    const char* targetStr = &argv[i][1];
    const char* wrapStr;
    const char* filterStr;
    const char* sizexStr;
    const char* sizeyStr;
    const char* sizezStr;

    if (strncmp(targetStr, "2d", 2) == 0) {
        texture.target = Texture::TEXTURE_2D;
    } else if (strncmp(targetStr, "3d", 2) == 0) {
        texture.target = Texture::TEXTURE_3D;
    } else {
        printf("malformed texture description '%s', it should look like [2d:linear:repeat]\n", argv[i]);
        return false;
    }

    filterStr = targetStr + 3;
    if (strncmp(filterStr, "linear", 6) == 0) {
        texture.filter = Texture::LINEAR;
        wrapStr = filterStr + 7;
    } else if (strncmp(filterStr, "nearest", 7) == 0) {
        texture.filter = Texture::NEAREST;
        wrapStr = filterStr + 8;
    } else if (strncmp(filterStr, "linear_mipmap_linear", 20) == 0) {
        texture.filter = Texture::LINEAR_MIPMAP_LINEAR;
        wrapStr = filterStr + 21;
    } else {
        printf("malformed texture description '%s', it should look like [2d:linear:repeat]\n", argv[i]);
        return false;
    }

    if (strncmp(wrapStr, "repeat", 6) == 0) {
        texture.wrap = Texture::REPEAT;
        sizexStr = wrapStr + 7;
    } else if (strncmp(wrapStr, "clamp", 5) == 0) {
        texture.wrap = Texture::CLAMP;
        sizexStr = wrapStr + 6;
    } else {
        printf("malformed texture description '%s', it should look like [2d:linear:repeat]\n", argv[i]);
        return false;
    }

    // if it's a 3d texture we need to parse the size
    if (texture.target == Texture::TEXTURE_3D) {
        const char* endSizeX = strchr(sizexStr, ':');
        if (endSizeX > endPtr) {
            printf("malformed texture description '%s', it should look like "
                   "[3d:linear:repeat:sizex:sizey:sizez]\n",
                   argv[i]);
            return false;
        }
        char sizeTmp[16];
        size_t sizeStr;
        sizeStr = size_t(endSizeX - sizexStr);
        memcpy(sizeTmp, sizexStr, sizeStr);
        sizeTmp[sizeStr] = 0;
        texture.size[0] = atoi(sizeTmp);

        sizeyStr = endSizeX + 1;
        const char* endSizeY = strchr(sizeyStr, ':');
        if (endSizeY > endPtr) {
            printf("malformed texture description '%s', it should look like "
                   "[3d:linear:repeat:sizex:sizey:sizez]\n",
                   argv[i]);
            return false;
        }
        sizeStr = size_t(endSizeY - sizeyStr);
        memcpy(sizeTmp, sizeyStr, sizeStr);
        sizeTmp[sizeStr] = 0;
        texture.size[1] = atoi(sizeTmp);

        sizezStr = endSizeY + 1;
        const char* endSizeZ = endPtr;

        sizeStr = size_t(endSizeZ - sizezStr);
        memcpy(sizeTmp, sizeyStr, sizeStr);
        sizeTmp[sizeStr] = 0;
        texture.size[2] = atoi(sizeTmp);
    }

    return true;
}

std::string createFragmentTemplate(const WatchFileList& fileList)
{
    std::string fragmentTemplate = R"(
#version 330

out vec4 frag_colour;
uniform vec4 iMouse;
uniform vec3 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform float iFrameRate;
uniform vec3 iChannelResolution[4];
)";
    for (auto&& it : fileList) {
        char tmp[128];
        switch (it.type) {
        case WatchFile::SHADER:
            break;
        case WatchFile::TEXTURE0:
        case WatchFile::TEXTURE1:
        case WatchFile::TEXTURE2:
        case WatchFile::TEXTURE3:
            const int textureIndex = it.type - int(WatchFile::TEXTURE0);
            const int index = sprintf(tmp, "uniform sampler%s iChannel%d;\n",
                                      it.texture.target == Texture::TEXTURE_2D ? "2D" : "3D", textureIndex);
            tmp[index] = 0;
            fragmentTemplate += std::string(tmp);
            break;
        }
    }
    return fragmentTemplate;
}

int main(int argc, const char** argv)
{
    (void)argc;
    (void)argv;
    const char* const saveImagePath = "./shaderjoy_frame.png";
    bool executeOneFrame = false;
    initTime();
    Application app;

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage();
        return 0;
    }

    if (argc > 1) {
        int shaderIndex = -1;
        for (int i = 1; i < argc; i++) {

            if (strcmp(argv[i], "--save-frame") == 0) {
                executeOneFrame = true;
                printf("will execute and save one frame [%s]\n", saveImagePath);

                // handle argument texture like:
                // --texture0 [2d:linear:repeat] file.png
                // --texture0 [3d:linear:repeat:sizex:sizey:sizez] file
            } else if (strncmp(argv[i], "--texture", 9) == 0) {
                int textureIndex = argv[i][9] - '0';
                WatchFile fileEntry;
                fileEntry.type = WatchFile::Type(WatchFile::TEXTURE0 + textureIndex);
                i++;
                // check if there is a config section [2d:linear:wrap]
                if (argv[i][0] == '[') {
                    if (i + 1 >= argc) {
                        printf("not enough arguemnt to parse --texture0, expect '[2d:linear:clamp] "
                               "texture_file.jpg'\n");
                        return 1;
                    }
                    if (!parseTextureBlock(argv, i, fileEntry.texture)) {
                        return 1;
                    }
                    i++;
                }
                fileEntry.path = argv[i];
                app.watcher._files.push_back(fileEntry);
            } else {
                shaderIndex = i;
                break;
            }
        }

        if (shaderIndex != -1) {
            printf("watching file %s\n", argv[shaderIndex]);
            app.watcher._files.push_back(WatchFile(WatchFile::SHADER, argv[shaderIndex]));
        }

    } else {
        printf("no file to watch use the default shader as example\n");
    }

    // dump files from
    for (auto&& entry : app.watcher._files) {
        dumpFileEntry(entry);
    }

    glfwSetErrorCallback(outputError);

    if (!glfwInit()) {
        printf("GLFW init failed\n");
        return 1;
    }

    GLFWwindow* window = setupWindow(REGULAR, &app);

    if (!window) {
        return 1;
    }

    initIMGUI(window);

    // setup default fragmentProgram
    // define texture configurations
    const std::string fragmentTemplate = createFragmentTemplate(app.watcher._files);
    defaultTemplatePreFragment = fragmentTemplate.c_str();

    // fullscreen triangle
    float points[] = {4.0f,  -1.0f, // NOLINT
                      -1.0f, 4.0f,  // NOLINT
                      -1.0f, -1.0f};
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), points, GL_STATIC_DRAW);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint vs;
    GLuint fs = -1U;
    GLuint program = -1U;
    ProgramDescription programDescription;
    compileShader(defaultVertex, GL_VERTEX_SHADER, vs);
    if (!compileProgram(vs, defaultFragment, strlen(defaultFragment), fs, program, app.shaderReport)) {
        return 1;
    }
    getProgramDescription(program, programDescription);
    UniformList uniformList;
    getUniformList(&programDescription, uniformList);

    app.running.store(true);
    std::thread fileWatcher(&fileWatcherThread, &app);

    double timeStart = getTimeInMS();
    double lastFrame = timeStart;

    double fpsStart = timeStart;
    int fpsFrameCount = 0;

    GLuint textures[4] = {~0x0u, ~0x0u, ~0x0u, ~0x0u};

    while (app.running.load() && !glfwWindowShouldClose(window)) {

        /* Poll for and process events */
        glfwPollEvents();

        if (app.watcher.fileChanged()) {
            app.watcher.lock();

            // be careful do not use 'changedFile' after unlocking
            const WatchFile& changedFile = app.watcher.getChangedFile();
            switch (changedFile.type) {
            case WatchFile::SHADER: {
                bool status = compileProgram(vs, reinterpret_cast<const char*>(changedFile.data.data()),
                                             changedFile.data.size(), fs, program, app.shaderReport);
                app.watcher.resetFileChanged();
                app.watcher.unlock();
                if (status) {
                    ProgramDescription newProgramDescription;
                    getProgramDescription(program, newProgramDescription);
                    UniformList newList;
                    getUniformList(&newProgramDescription, newList);
                    uniformList = newList;
                }
                break;
            }
            case WatchFile::TEXTURE0:
            case WatchFile::TEXTURE1:
            case WatchFile::TEXTURE2:
            case WatchFile::TEXTURE3: {
                int textureIndex = changedFile.type - int(WatchFile::TEXTURE0);
                updateTexture(textures[textureIndex], uniformList.iChannelResolution[textureIndex],
                              changedFile.texture);
                app.watcher.resetFileChanged();
                app.watcher.unlock();
                break;
            }
            }
            app.requestFrame = true;
        }

        // the app can be in pause in this case we do not render new frame
        // except if a requestFrame is asked. It happens when resizing the window
        // or recompiling a program
        // so requestFrame is a a boolean used as an exception when in pause mode
        if (app.pause && !app.requestFrame) {
            sleepInMS(500);
            continue;
        }

        float mouseX, mouseY;
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            mouseX = static_cast<float>(xpos);
            mouseY = static_cast<float>(ypos);
        }

        const float viewportWidth = app.width * app.pixelRatio;
        const float viewportHeight = app.height * app.pixelRatio;
        mouseX = clamp(mouseX, 0.0f, viewportWidth);
        mouseY = clamp(mouseY, 0.0f, viewportHeight);

        // update iMouse
        if (app.mouseButtonClicked[0]) {
            uniformList.iMouse[0] = mouseX;
            uniformList.iMouse[1] = mouseY;
        }
        if (app.mouseButtonClicked[1]) {
            uniformList.iMouse[2] = mouseX;
            uniformList.iMouse[3] = mouseY;
        }

        // update window dimension
        uniformList.iResolution[0] = viewportWidth;
        uniformList.iResolution[1] = viewportHeight;
        uniformList.iResolution[2] = viewportHeight / viewportWidth;

        // Clear the background
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glUseProgram(program);

        for (int textureIndex = 0; textureIndex < 4; textureIndex++) {
            if (textures[textureIndex] != ~0x0u) {
                glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(textureIndex));
                glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);
            }
        }

        updateUniforms(uniformList);

        glBindVertexArray(vao);
        // draw points 0-3 from the currently bound VAO with current in-use shader
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // do not save the ui if execute and save one frame
        if (!executeOneFrame) {
            frameIMGUI(&app, uniformList);
        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        if (executeOneFrame) {
            screenShoot(&app, saveImagePath);
            // screenShoot(&app, saveImagePath);
            break;
        }

        // updates some var to refresh uniforms
        uniformList.iFrame++;
        fpsFrameCount++;
        {
            const double now = getTimeInMS();
            uniformList.iTime = float((now - timeStart) / 1000.0);
            uniformList.iTimeDelta = float((now - lastFrame) / 1000.0);
            lastFrame = now;

            // compute fps
            const double deltaFPS = now - fpsStart;
            if (deltaFPS >= 1000.0) {
                app.frameRate = static_cast<float>(double(fpsFrameCount) * 1000.0 / deltaFPS);
                fpsFrameCount = 0;
                fpsStart = now;
            }
        }

        // reset the request of frame
        app.requestFrame = false;
    }
    app.running.store(false);

    fileWatcher.join();

    cleanupIMGUI();
    cleanupWindow(window);

    return 0;
}
