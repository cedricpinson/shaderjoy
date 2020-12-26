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

const char* defaultTemplatePreFragment = R"(
#version 330

out vec4 frag_colour;

uniform vec4 iMouse;
uniform vec3 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform float iFrameRate;

)";

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
    const size_t bufferSize = 16384;
    char tmpBuffer[bufferSize];

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
        char* infoLog = shaderReport ? shaderReport->errorBuffer.data() : tmpBuffer;
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

            const size_t shaderTextSize = generateShaderTextWithErrorsInlined(*shaderReport, tmpBuffer, printConsole);
            (void)shaderTextSize;
            assert(shaderTextSize < bufferSize && "BufferSize too small to report shader errors");
            printf("shader failed to compile:\n%s\n", tmpBuffer);

            const size_t errorTextSize = generateShaderTextErrors(*shaderReport, tmpBuffer);
            (void)errorTextSize;
            assert(errorTextSize < bufferSize && "BufferSize too small to report shader errors");
            printf("\nerrors lists:\n%s\n", tmpBuffer);
        } else {
            const size_t shaderTextSize = generateShaderTextWithErrorsInlined(*shaderReport, tmpBuffer, printConsole);
            (void)shaderTextSize;
            assert(shaderTextSize < bufferSize && "BufferSize too small to display shader");
            printf("%s\n", tmpBuffer);
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

void getUniformList(const ProgramDescription* description, UniformList& uniforms)
{
    uniforms.iMouseLocation = getUniformLocation(description, "iMouse");
    uniforms.iTimeLocation = getUniformLocation(description, "iTime");
    uniforms.iResolutionLocation = getUniformLocation(description, "iResolution");
    uniforms.iTimeDeltaLocation = getUniformLocation(description, "iTimeDelta");
    uniforms.iFrameLocation = getUniformLocation(description, "iFrame");
}

//#define DISPLAY_UNIFORM

void updateUniforms(UniformList uniforms)
{

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
    printf("Usage: shaderjoy [--save-frame] shader-file.glsl\n");
    printf("run shaderjoy and watch files to reload them if they change\n");
    printf("to report issue: https://github.com/cedricpinson/shaderjoy/issues\n");
}

int main(int argc, char** argv)
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

    int argvIndex = 1;
    if (argc > 1) {
        if (strcmp(argv[argvIndex], "--save-frame") == 0) {
            executeOneFrame = true;
            argvIndex++;
            printf("will execute and save one frame [%s]\n", saveImagePath);
        }
        printf("watching file %s\n", argv[argvIndex]);
        app.files.push_back(argv[argvIndex]);
    } else {
        printf("no file to watch use the default shader as example\n");
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

    while (app.running.load() && !glfwWindowShouldClose(window)) {

        /* Poll for and process events */
        glfwPollEvents();

        if (app.watcher.fileChanged()) {
            app.watcher.lock();

            bool status = compileProgram(vs, app.watcher.getData().data(), app.watcher.getData().size(), fs, program,
                                         app.shaderReport);
            app.watcher.resetFileChanged();
            app.watcher.unlock();
            if (status) {
                ProgramDescription newProgramDescription;
                getProgramDescription(program, newProgramDescription);
                UniformList newList;
                getUniformList(&newProgramDescription, newList);
                uniformList = newList;
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
