// SimpleDepthShader.inl
#pragma once

inline const char* simpleDepthShaderSource = R"(
#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

uniform bool useCameraProjView;

uniform mat4 projView;
uniform mat4 model;

uniform bool animated;
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

vec4 applyBoneTransform(vec4 pos)
{
    vec4 result = vec4(0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (aBoneIDs[i] == -1)
            continue;
        if (aBoneIDs[i] >= MAX_BONES)
        {
            result = pos;
            break;
        }
        result += aBoneWeights[i] * (finalBonesMatrices[aBoneIDs[i]] * pos);
    }
    return result;
}

void main()
{
    vec4 totalPosition = vec4(0.0);

    if (animated)
    {
        totalPosition = applyBoneTransform(vec4(aPosition, 1.0f));
    }
    else
    {
        totalPosition = vec4(aPosition, 1.0f);
    }

    vec3 worldPos = vec3(model * totalPosition);

    if (useCameraProjView)
        gl_Position = projection * view * vec4(worldPos, 1.0f);
    else
        gl_Position = projView * model * totalPosition;
    
    //gl_Position.z += 0.001f;
}

#[fragment]

#version 450 core

void main() 
{
}
)";