// SimpleDepthShader.inl
#pragma once

inline const char* simpleDepthShaderSource = R"(
#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

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

    gl_Position = projView * model * totalPosition;
}

#[fragment]

#version 450 core

void main() 
{
}
)";