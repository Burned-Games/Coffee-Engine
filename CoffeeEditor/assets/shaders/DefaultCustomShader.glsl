// DefaultCustomShader.inl
#pragma once

inline const char* defaultCustomShaderSource = R"(
#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

struct VertexData
{
    vec2 TexCoords;
    vec3 WorldPos;
    vec3 camPos;
};

layout (location = 2) out VertexData Output;

uniform mat4 model;

void main()
{
    Output.TexCoords = aTexCoord;
    Output.WorldPos = vec3(model * vec4(aPosition, 1.0));
    Output.camPos = cameraPos;

    gl_Position = projection * view * vec4(Output.WorldPos, 1.0);
}

#[fragment]

#version 450 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 EntityID;

uniform vec3 entityID;

struct VertexData
{
    vec2 TexCoords;
    vec3 WorldPos;
    vec3 camPos;
};

layout (location = 2) in VertexData VertexInput;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    EntityID = vec4(entityID, 1.0f);
}
)";