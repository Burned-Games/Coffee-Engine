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

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}

#[fragment]

#version 450 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 EntityID;

in vec2 TexCoord;

layout(std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

float PristineGrid(vec2 uv, float lineWidth) 
{
    vec4 uvDDXY = vec4(dFdx(uv), dFdy(uv));
    vec2 uvDeriv = vec2(length(uvDDXY.xz), length(uvDDXY.yw));
    bvec2 invertLine = greaterThan(vec2(lineWidth), vec2(0.5));
    vec2 targetWidth = mix(vec2(lineWidth), vec2(1.0 - lineWidth), invertLine);
    vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
    vec2 lineAA = uvDeriv * 1.5;
    vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
    gridUV = mix(1.0 - gridUV, gridUV, invertLine);
    vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(targetWidth / drawWidth, vec2(0.0), vec2(1.0));
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, vec2(0.0), vec2(1.0)));
    grid2 = mix(grid2, 1.0 - grid2, invertLine);
    return mix(grid2.x, 1.0, grid2.y);
}

void main()
{
    vec3 worldPos = vec3(0.0, 0.0, 0.0);
    vec2 worldXZ = TexCoord * 1000.0;

    float distance = length(cameraPos - worldPos);

    //float fade = clamp(1.0 - distance / 100.0, 0.0, 1.0);

    // --- Grid lines ---
    float divs = 1.0;
    float grid1 = PristineGrid(worldXZ * divs, 0.02);
    float grid2 = PristineGrid(worldXZ * divs / 10.0, 0.02);

    // --- Blend grids based on distance ---
    float blend = smoothstep(10.0, 50.0, distance);
    float gridValue = mix(grid1, grid2, blend);

    // --- Output ---
    vec3 color = vec3(0.45);
    FragColor = vec4(color, gridValue * 0.5/*  * fade */); // Fade affects grid alpha
    EntityID = vec4(1.0);
}
