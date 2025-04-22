// ShadowShader.inl
#pragma once

inline const char* shadowShaderSource = R"(
#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPosition, 1.0);
} 

#[fragment]

#version 450 core

void main() 
{
}
)";