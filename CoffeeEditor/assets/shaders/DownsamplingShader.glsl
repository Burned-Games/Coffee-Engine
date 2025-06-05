// This shader performs downsampling on a texture,
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

uniform sampler2D srcTexture;
uniform int srcMipLevel;

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

void main()
{
    vec3 downsample = vec3(0.0);

    // Use textureSize to get the size of the specific mip level
    vec2 srcTexelSize = 1.0 / textureSize(srcTexture, srcMipLevel);
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = textureLod(srcTexture, vec2(TexCoord.x - 2*x, TexCoord.y + 2*y), srcMipLevel).rgb;
    vec3 b = textureLod(srcTexture, vec2(TexCoord.x,       TexCoord.y + 2*y), srcMipLevel).rgb;
    vec3 c = textureLod(srcTexture, vec2(TexCoord.x + 2*x, TexCoord.y + 2*y), srcMipLevel).rgb;

    vec3 d = textureLod(srcTexture, vec2(TexCoord.x - 2*x, TexCoord.y), srcMipLevel).rgb;
    vec3 e = textureLod(srcTexture, vec2(TexCoord.x,       TexCoord.y), srcMipLevel).rgb;
    vec3 f = textureLod(srcTexture, vec2(TexCoord.x + 2*x, TexCoord.y), srcMipLevel).rgb;

    vec3 g = textureLod(srcTexture, vec2(TexCoord.x - 2*x, TexCoord.y - 2*y), srcMipLevel).rgb;
    vec3 h = textureLod(srcTexture, vec2(TexCoord.x,       TexCoord.y - 2*y), srcMipLevel).rgb;
    vec3 i = textureLod(srcTexture, vec2(TexCoord.x + 2*x, TexCoord.y - 2*y), srcMipLevel).rgb;

    vec3 j = textureLod(srcTexture, vec2(TexCoord.x - x, TexCoord.y + y), srcMipLevel).rgb;
    vec3 k = textureLod(srcTexture, vec2(TexCoord.x + x, TexCoord.y + y), srcMipLevel).rgb;
    vec3 l = textureLod(srcTexture, vec2(TexCoord.x - x, TexCoord.y - y), srcMipLevel).rgb;
    vec3 m = textureLod(srcTexture, vec2(TexCoord.x + x, TexCoord.y - y), srcMipLevel).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    FragColor = vec4(downsample, 1.0);
}