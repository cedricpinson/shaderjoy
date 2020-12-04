#include "Application.h"
#include "ProgramDescription.h"
#include "UniformList.h"
#include "glad/glad.h"
#include "timer.h"
#include "watcher.h"
#include "window.h"

#include <sys/stat.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

void initIMGUI(GLFWwindow* window);
void drawIMGUI();
void cleanupIMGUI();

void debugShader(const char* text, const char* error);

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

const char* defaultTemplateFragment1 = R"(
#version 330

out vec4 frag_colour;

uniform vec3 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform float iFrameRate;

)";

const char* defaultTemplateFragment2 = R"(
void main() {

  vec4 color;
  mainImage(color, gl_FragCoord.xy);
  frag_colour = color;
}

)";

bool compileShader(const char* shaderText, GLenum shaderType, GLuint& shader)
{
    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderText, NULL);
    glCompileShader(shader);
    GLint shaderResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderResult);
    char infoLog[8192];
    if (!shaderResult) {
        printf("fails to compile shader:\n");
        char buffer[4096];
        GLsizei size;
        glGetShaderInfoLog(shader, 4096, &size, buffer);
        buffer[size] = 0;
        debugShader(buffer, infoLog);
        printf("%s\n", infoLog);
        return false;
    }
    return true;
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

bool compileProgram(const GLuint vs, const char* shader, const size_t size, GLuint& fs, GLuint& program)
{
    std::vector<char> fullShader;
    fullShader.resize(size + 1024);
    size_t index = 0;
    size_t fragmentSize = 0;
    fragmentSize = strlen(defaultTemplateFragment1);
    memcpy(fullShader.data() + index, defaultTemplateFragment1, fragmentSize);
    index += fragmentSize;
    memcpy(fullShader.data() + index, shader, size);
    index += size;
    fragmentSize = strlen(defaultTemplateFragment2);
    memcpy(fullShader.data() + index, defaultTemplateFragment2, fragmentSize);
    index += fragmentSize;
    fullShader[index + 1] = 0;
    fullShader.resize(index + 1);

    GLuint newFS = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fullShader.data(), GL_FRAGMENT_SHADER, newFS)) {
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

    // success and exist an older program delete it
    if (program != -1U) {
        glDeleteProgram(program);
        glDeleteShader(fs);
    }

    program = newProgram;
    fs = newFS;

    printf("%s\n", shader);
    return true;
}

void getUniformList(const ProgramDescription* description, UniformList& uniforms)
{
    uniforms.iTimeLocation = getUniformLocation(description, "iTime");
    uniforms.iResolutionLocation = getUniformLocation(description, "iResolution");
    uniforms.iTimeDeltaLocation = getUniformLocation(description, "iTimeDelta");
    uniforms.iFrameLocation = getUniformLocation(description, "iFrame");
    uniforms.iFrameRateLocation = getUniformLocation(description, "iFrameRate");
}

//#define DISPLAY_UNIFORM

void updateUniforms(UniformList uniforms)
{
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

    if (uniforms.iFrameRateLocation != -1) {
#ifdef DISPLAY_UNIFORM
        printf("iFrameRate %d\n", uniforms.iFrameRate);
#endif
        glUniform1f(uniforms.iFrameRateLocation, uniforms.iFrameRate);
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

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    initTime();
    Application app;

    if (argc > 1) {
        app.files.push_back(argv[1]);
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
    if (!compileProgram(vs, defaultFragment, strlen(defaultFragment), fs, program)) {
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

            bool status = compileProgram(vs, app.watcher.getData().data(), app.watcher.getData().size(), fs, program);
            app.watcher.resetFileChanged();
            app.watcher.unlock();
            if (status) {
                ProgramDescription newProgramDescription;
                getProgramDescription(program, newProgramDescription);
                UniformList newList;
                getUniformList(&newProgramDescription, newList);
                uniformList = newList;
            } else {
                printf("error to compile new program\n");
            }
        }

        // the app can be in pause in this case we do not render new frame
        // except if a requestFrame is asked. It happens when resizing the window
        // or recompiling a program
        // so requestFrame is a a boolean used as an exception when in pause mode
        if (app.pause && !app.requestFrame) {
            sleepInMS(500);
            continue;
        }

        // update window dimension
        uniformList.iResolution[0] = float(app.width) * app.pixelRatio;
        uniformList.iResolution[1] = float(app.height) * app.pixelRatio;
        uniformList.iResolution[2] = uniformList.iResolution[1] / uniformList.iResolution[0];

        // Clear the background
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glUseProgram(program);

        updateUniforms(uniformList);

        glBindVertexArray(vao);
        // draw points 0-3 from the currently bound VAO with current in-use shader
        glDrawArrays(GL_TRIANGLES, 0, 3);

        frameIMGUI(&app, uniformList);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

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
                uniformList.iFrameRate = static_cast<float>(double(fpsFrameCount) * 1000.0 / deltaFPS);
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
