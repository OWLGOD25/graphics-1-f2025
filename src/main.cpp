#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Window.h"
#include "Shader.h"
#include <iostream>
#include <vector>
#include <cmath>

// Vertex shader - supports position + per-vertex color and simple transforms based on mode
static const char* vertexSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

uniform int mode;      // indicates which triangle behavior to apply
uniform float time;    // global time
uniform vec2 offset;   // translation offset for mode 3
uniform float angle;   // rotation angle for mode 4
uniform vec2 center;   // center for rotations/translations if needed

out vec3 vColor;

void main()
{
    vec2 pos = aPos;

    if (mode == 3) {
        // translation triangle: translate by offset
        pos += offset;
    }
    else if (mode == 4) {
        // rotate about the provided center
        vec2 p = pos - center;
        float s = sin(angle);
        float c = cos(angle);
        p = vec2(c*p.x - s*p.y, s*p.x + c*p.y);
        pos = p + center;
    }

    gl_Position = vec4(pos, 0.0, 1.0);
    vColor = aColor;
}
)";

// Fragment shader - supports pulsing color when mode==2
static const char* fragmentSrc = R"(
#version 430 core
in vec3 vColor;
uniform int mode;
uniform float time;

out vec4 FragColor;

void main()
{
    vec3 color = vColor;
    if (mode == 2) {
        // color changes over time (pulse)
        float t = 0.5 + 0.5 * sin(time * 2.0); // ranges [0,1]
        color = color * (0.25 + 0.75 * t);
    }
    FragColor = vec4(color, 1.0);
}
)";

// helper to create VAO/VBO from position vec2 and color vec3 per-vertex
struct VAOHandle {
    GLuint vao = 0;
    GLuint vbo = 0;
};

VAOHandle CreateTriangle(const std::vector<float>& interleavedData)
{
    VAOHandle h;
    glGenVertexArrays(1, &h.vao);
    glGenBuffers(1, &h.vbo);

    glBindVertexArray(h.vao);
    glBindBuffer(GL_ARRAY_BUFFER, h.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);

    // layout(location=0) vec2 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);

    // layout(location=1) vec3 color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 2));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return h;
}

void DestroyTriangle(VAOHandle& h)
{
    if (h.vbo) { glDeleteBuffers(1, &h.vbo); h.vbo = 0; }
    if (h.vao) { glDeleteVertexArrays(1, &h.vao); h.vao = 0; }
}

int main()
{
    CreateWindow(800, 800, "Graphics 1");

    // create shader
    Shader shader;
    std::string err;
    if (!shader.CreateFromSource(vertexSrc, fragmentSrc, err)) {
        std::cerr << "Shader compile/link error:\n" << err << std::endl;
        DestroyWindow();
        return -1;
    }

    // Triangles will be positioned vertically down the screen so you can see them all:
    // Top y = 0.75, next 0.35, -0.05, -0.45, -0.85
    // We'll use small triangles (height ~0.25) so they don't overlap.

    // 1) White triangle (mode 0)
    std::vector<float> white = {
        // pos.x, pos.y,  r, g, b
         -0.2f,  0.85f,  1.0f, 1.0f, 1.0f,
          0.2f,  0.85f,  1.0f, 1.0f, 1.0f,
          0.0f,  0.60f,  1.0f, 1.0f, 1.0f
    };

    // 2) Rainbow triangle (mode 1) - per-vertex color
    std::vector<float> rainbow = {
        -0.2f,  0.45f,   1.0f, 0.0f, 0.0f, // red
         0.2f,  0.45f,   0.0f, 1.0f, 0.0f, // green
         0.0f,  0.20f,   0.0f, 0.0f, 1.0f  // blue
    };

    // 3) Pulsing color (mode 2) - base color is magenta
    std::vector<float> pulsing = {
        -0.2f, -0.05f,   0.94f, 0.53f, 0.75f, // pastel magenta-ish
         0.2f, -0.05f,   0.94f, 0.53f, 0.75f,
         0.0f, -0.30f,   0.94f, 0.53f, 0.75f
    };

    // 4) Translating triangle (mode 3) - place around y = -0.55
    // Positions are centered so when offset is applied it moves left-right
    std::vector<float> translating = {
        -0.15f, -0.35f,  0.2f, 0.8f, 0.2f, // light green
         0.15f, -0.35f,  0.2f, 0.8f, 0.2f,
         0.0f, -0.60f,   0.2f, 0.8f, 0.2f
    };

    // 5) Rotating triangle (mode 4) - center about its own center near bottom
    // We'll provide a center uniform so rotation is around the triangle's center
    std::vector<float> rotating = {
        -0.25f, -0.75f,  1.0f, 0.6f, 0.2f, // orange
         0.25f, -0.75f,  1.0f, 0.6f, 0.2f,
         0.0f,  -0.55f,  1.0f, 0.6f, 0.2f
    };

    // Create VAOs
    VAOHandle vaoWhite = CreateTriangle(white);
    VAOHandle vaoRainbow = CreateTriangle(rainbow);
    VAOHandle vaoPulsing = CreateTriangle(pulsing);
    VAOHandle vaoTrans = CreateTriangle(translating);
    VAOHandle vaoRot = CreateTriangle(rotating);

    // Get uniform locations
    shader.Use();
    GLint locMode = glGetUniformLocation(shader.GetID(), "mode");
    GLint locTime = glGetUniformLocation(shader.GetID(), "time");
    GLint locOffset = glGetUniformLocation(shader.GetID(), "offset");
    GLint locAngle = glGetUniformLocation(shader.GetID(), "angle");
    GLint locCenter = glGetUniformLocation(shader.GetID(), "center");

    // render loop
    while (!WindowShouldClose())
    {
        float r = 239.0f / 255.0f;
        float g = 136.0f / 255.0f;
        float b = 190.0f / 255.0f;
        float a = 1.0f;

        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Use();
        float t = (float)glfwGetTime();
        glUniform1f(locTime, t);

        // 1) white
        glUniform1i(locMode, 0);
        glBindVertexArray(vaoWhite.vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 2) rainbow (uses per-vertex color)
        glUniform1i(locMode, 1);
        glBindVertexArray(vaoRainbow.vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 3) pulsing color (shader multiplies by time)
        glUniform1i(locMode, 2);
        glBindVertexArray(vaoPulsing.vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 4) translating left-right between x = -1 and x = 1
        // we'll compute offset.x = sin(t) * 0.75 to keep it within bounds
        glUniform1i(locMode, 3);
        float translateAmount = sinf(t * 1.2f) * 0.75f; // speed multiplier 1.2
        // only x translation needed
        glUniform2f(locOffset, translateAmount, 0.0f);
        glBindVertexArray(vaoTrans.vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 5) rotating CCW about z-axis
        glUniform1i(locMode, 4);
        // compute rotation angle - rotate counter-clockwise: angle increases with time
        float ang = t * 1.0f; // 1 radian per second
        glUniform1f(locAngle, ang);
        // center of rotation = approximate center of the triangle vertices used above
        // we calculated the triangle roughly centered at x=0, y=-0.68 (est)
        // to be precise, calculate average of its positions:
        // here we pass a center that matches the geometry chosen above:
        glUniform2f(locCenter, 0.0f, -0.68f);
        glBindVertexArray(vaoRot.vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // unbind VAO
        glBindVertexArray(0);

        Loop();
    }

    // cleanup
    DestroyTriangle(vaoWhite);
    DestroyTriangle(vaoRainbow);
    DestroyTriangle(vaoPulsing);
    DestroyTriangle(vaoTrans);
    DestroyTriangle(vaoRot);

    shader.Destroy();
    DestroyWindow();
    return 0;
}
