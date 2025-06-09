// Based on Call Of Duty method, presented at ACM Siggraph 2014.

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

layout (location = 0) out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D sourceTexture;
uniform sampler2D downsamplingTexture;
uniform sampler2D upsamplingTexture;

uniform float filterRadius; // Radius for upsampling
uniform int mipmapLevel;

uniform float bloomIntensity; // Intensity of the bloom effect

#define MODE_COPY 0
#define MODE_DOWNSAMPLING 1
#define MODE_UPSAMPLING 2
#define MODE_COMPOSITION 3

uniform int mode;

vec3 DownsampleBox13(sampler2D tex, float lod, vec2 uv, vec2 texelSize)
{
    // Center
    vec3 A = textureLod(tex, uv, lod).rgb;
	
    texelSize *= 0.5f; // Sample from center of texels
    
	// Inner box
    vec3 B = textureLod(tex, uv + texelSize * vec2(-1.0f, -1.0f), lod).rgb;
    vec3 C = textureLod(tex, uv + texelSize * vec2(-1.0f, 1.0f), lod).rgb;
    vec3 D = textureLod(tex, uv + texelSize * vec2(1.0f, 1.0f), lod).rgb;
    vec3 E = textureLod(tex, uv + texelSize * vec2(1.0f, -1.0f), lod).rgb;
	
    // Outer box
    vec3 F = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 G = textureLod(tex, uv + texelSize * vec2(-2.0f, 0.0f), lod).rgb;
    vec3 H = textureLod(tex, uv + texelSize * vec2(0.0f, 2.0f), lod).rgb;
    vec3 I = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 J = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 K = textureLod(tex, uv + texelSize * vec2(2.0f, 0.0f), lod).rgb;
    vec3 L = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 M = textureLod(tex, uv + texelSize * vec2(0.0f, -2.0f), lod).rgb;
	
    // Weights
    vec3 result = vec3(0.0);
    {
        // Inner box
        result += (B + C + D + E) * 0.5f;
        // Bottom-left box
        result += (F + G + A + M) * 0.125f;
        // Top-left box
        result += (G + H + I + A) * 0.125f;
        // Top-right box
        result += (A + I + J + K) * 0.125f;
        // Bottom-right box
        result += (M + A + K + L) * 0.125f;
        
        // 4 samples each
        result *= 0.25f;
    }

    return result;
}

vec3 UpsampleTent9(sampler2D tex, float lod, vec2 uv, vec2 texelSize, float radius)
{
    vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;
	
    // Center
    vec3 result = textureLod(tex, uv, lod).rgb * 4.0f;
	
    result += textureLod(tex, uv - offset.xy, lod).rgb;
    result += textureLod(tex, uv - offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv - offset.zy, lod).rgb;
	
    result += textureLod(tex, uv + offset.zw, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xw, lod).rgb * 2.0;
	
    result += textureLod(tex, uv + offset.zy, lod).rgb;
    result += textureLod(tex, uv + offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xy, lod).rgb;
	
    return result * (1.0f / 16.0f);
}

void main()
{
    if (mode == MODE_COPY)
    {
        FragColor = textureLod(sourceTexture, TexCoord, float(mipmapLevel));
        //FragColor.rgb = UpsampleTent9(sourceTexture, mipmapLevel, TexCoord, 1.0f / vec2(textureSize(sourceTexture, mipmapLevel)), filterRadius);
        FragColor.a = 1.0f;
    }
    else if (mode == MODE_DOWNSAMPLING)
    {
        vec2 texelSize = 1.0f / textureSize(downsamplingTexture, mipmapLevel);
        FragColor.rgb = DownsampleBox13(downsamplingTexture, mipmapLevel - 1, gl_FragCoord.xy / vec2(textureSize(downsamplingTexture, mipmapLevel)), texelSize);
        FragColor.a = 1.0f;
    }
    else if (mode == MODE_UPSAMPLING)
    {
        int prevMip = mipmapLevel + 1;

        // Calculate UVs in [0,1] for the current mip (higher res)
        vec2 uv = gl_FragCoord.xy / vec2(textureSize(upsamplingTexture, mipmapLevel));

        // Calculate texel size for the previous mip (lower res)
        vec2 texelSize = 1.0 / vec2(textureSize(upsamplingTexture, prevMip));

        // Upsample from previous mip at the current UV
        vec3 upsampled = UpsampleTent9(upsamplingTexture, prevMip, uv, texelSize, filterRadius);

        // Sample the current downsampled mip at the current UV
        vec3 downsampled = textureLod(downsamplingTexture, uv, mipmapLevel).rgb;
        downsampled = max(downsampled, 0.0001f);

        FragColor.rgb = upsampled + downsampled;
        FragColor.a = 1.0f;
    }
    else if (mode == MODE_COMPOSITION)
    {
        // Blend the upsampled texture with the source texture
        vec3 upsampled = textureLod(upsamplingTexture, TexCoord, 0).rgb;
        //vec3 upsampled = UpsampleTent9(upsamplingTexture, 0, TexCoord, 1.0f / vec2(textureSize(sourceTexture, 0)), filterRadius);
        vec3 source = textureLod(sourceTexture, TexCoord, 0).rgb;

        //FragColor.rgb = mix(source, upsampled, bloomIntensity);
        FragColor.rgb = source + upsampled * bloomIntensity;
        FragColor.a = 1.0f;
        //FragColor = vec4(upsampled, 1.0);
    }
    else
    {
        FragColor = vec4(0.0f); // Fallback to black if mode is invalid
    }
}