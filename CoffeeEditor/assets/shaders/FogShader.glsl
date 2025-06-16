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

uniform sampler2D colorTexture;
uniform sampler2D depthTexture;

// Fog controls
uniform vec3 FogColor;           // Base fog color (e.g. blue/gray)
uniform float FogDensity;        // Exponential fog density
uniform float FogHeight;         // Height at which fog starts
uniform float FogHeightDensity;  // Height fog density

// Camera block (std140, binding = 0)
layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

// Light data (assuming at least one directional light is sun)
#define MAX_LIGHTS 32
struct Light
{
    vec3 color;
    vec3 direction;
    vec3 position;
    float range;
    float attenuation;
    float intensity;
    float angle;
    int type;
    bool shadow;
    float shadowBias;
    float shadowMaxDistance;
};
layout (std140, binding = 1) uniform RenderData
{
    Light lights[MAX_LIGHTS];
    int lightCount;
    mat4 lightSpaceMatrices[4];
};

uniform mat4 invProjection; // Inverse of projection matrix
uniform mat4 invView; // Inverse of view matrix

vec3 ReconstructViewPos(vec2 uv, float depth)
{
    float x = uv.x * 2.0 - 1.0;
    float y = uv.y * 2.0 - 1.0;
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(x, y, z, 1.0);
    vec4 viewPos = invProjection * clip;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    vec3 color = texture(colorTexture, TexCoord).rgb;
    float depth = texture(depthTexture, TexCoord).r;

    vec3 viewPos = ReconstructViewPos(TexCoord, depth);
    float dist = length(viewPos);

    // Reconstruct world position
    vec4 worldPos4 = invView * vec4(viewPos, 1.0);
    vec3 worldPos = worldPos4.xyz / worldPos4.w;

    // Exponential fog
    float fogAmount = 1.0 - exp(-dist * FogDensity);

    // Optional physically-based height fog
    if (abs(FogHeightDensity) >= 0.0001) {
        // Transform view-space position to world-space using invView
        float y = worldPos4.y;
        float y_dist = y - FogHeight;
        // Physically-based height fog
        float vfog_amount = 1.0 - exp(min(0.0, y_dist * FogHeightDensity));
        fogAmount = max(vfog_amount, fogAmount);
    }

    // Sun scattering in fog (use first directional light as sun)
    vec3 fogCol = FogColor;

    for (int i = 0; i < lightCount; ++i) {
        if (lights[i].type == 0) { // Directional light
            vec3 sunDir = normalize(lights[i].direction); // In world space
            vec3 viewDirWorld = normalize(worldPos - cameraPos); // Ray from camera to fragment in world space
            float sunAmount = max(dot(viewDirWorld, -sunDir), 0.0);
            fogCol = mix(FogColor, lights[i].color, pow(sunAmount, 8.0));
            break;
        }
    }

    color = mix(color, fogCol, clamp(fogAmount, 0.0, 1.0));

    FragColor = vec4(color, 1.0);
}