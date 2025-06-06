// QuadShader.inl
#pragma once

const char* quadShaderSource = R""(
#[vertex]

#version 450 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTexIndex;
layout (location = 4) in float aTilingFactor;
layout (location = 5) in vec3 aEntityID;

layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

struct VertexData
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout (location = 0) out VertexData Output;
layout (location = 3) out flat float TexIndex;
layout (location = 4) out flat vec3 entityID;

void main()
{
    Output.Color = aColor;
    Output.TexCoord = aTexCoord;
    Output.TilingFactor = aTilingFactor;
    TexIndex = aTexIndex;
    entityID = aEntityID;

    gl_Position = projection * view * vec4(aPosition, 1.0);
}

#[fragment]
#version 450 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 EntityID;

struct VertexData
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout (location = 0) in VertexData Input;
layout (location = 3) in flat float TexIndex;
layout (location = 4) in flat vec3 entityID;

layout (binding = 0) uniform sampler2D uTextures[32];

void main()
{
    vec4 color = texture(uTextures[int(TexIndex)], Input.TexCoord * Input.TilingFactor) * Input.Color;

    if (color.a < 0.1)
        discard;

    // gamma correct
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));

    FragColor = color;
    EntityID = vec4(entityID, 1.0f);
}
)"";