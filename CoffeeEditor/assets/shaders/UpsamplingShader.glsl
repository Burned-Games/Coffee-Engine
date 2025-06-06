// This shader performs upsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.

#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);
}

#[fragment]

#version 450 core

uniform sampler2D upsampleTexture;   // The upsampled (lower-res, higher mip) texture
uniform sampler2D downsampleTexture; // The downsampled (current mip) texture
uniform float filterRadius;
uniform int upsampleMipLevel;   // mip to upsample from (higher mip, lower res)
uniform int downsampleMipLevel; // mip to blend with (current mip, output res)
uniform bool firstPass; // Add this uniform

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

void main()
{
    if (firstPass) {
    // Only copy the highest mipmap (no blending)
    vec3 color = textureLod(downsampleTexture, TexCoord, downsampleMipLevel).rgb;
    FragColor = vec4(color, 1.0);
    return;
    }

    // Tent filter upsample from the higher mip
    vec2 texelSize = 1.0 / textureSize(upsampleTexture, upsampleMipLevel);
    float x = filterRadius * texelSize.x;
    float y = filterRadius * texelSize.y;

    vec3 a = textureLod(upsampleTexture, vec2(TexCoord.x - x, TexCoord.y + y), upsampleMipLevel).rgb;
    vec3 b = textureLod(upsampleTexture, vec2(TexCoord.x,     TexCoord.y + y), upsampleMipLevel).rgb;
    vec3 c = textureLod(upsampleTexture, vec2(TexCoord.x + x, TexCoord.y + y), upsampleMipLevel).rgb;

    vec3 d = textureLod(upsampleTexture, vec2(TexCoord.x - x, TexCoord.y), upsampleMipLevel).rgb;
    vec3 e = textureLod(upsampleTexture, vec2(TexCoord.x,     TexCoord.y), upsampleMipLevel).rgb;
    vec3 f = textureLod(upsampleTexture, vec2(TexCoord.x + x, TexCoord.y), upsampleMipLevel).rgb;

    vec3 g = textureLod(upsampleTexture, vec2(TexCoord.x - x, TexCoord.y - y), upsampleMipLevel).rgb;
    vec3 h = textureLod(upsampleTexture, vec2(TexCoord.x,     TexCoord.y - y), upsampleMipLevel).rgb;
    vec3 i = textureLod(upsampleTexture, vec2(TexCoord.x + x, TexCoord.y - y), upsampleMipLevel).rgb;

    vec3 upsampled = e*4.0;
    upsampled += (b+d+f+h)*2.0;
    upsampled += (a+c+g+i);
    upsampled *= 1.0 / 16.0;

    // Sample the current downsampled mip (same resolution as output)
    vec3 downsampled = textureLod(downsampleTexture, TexCoord, downsampleMipLevel).rgb;

    // Add (or blend) the upsampled and downsampled results
    vec3 result = upsampled + downsampled;

    FragColor = vec4(result, 1.0);
}