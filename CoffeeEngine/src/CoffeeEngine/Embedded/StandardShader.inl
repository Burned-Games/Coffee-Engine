// StandardShader.inl
#pragma once

const char* standardShaderSource = R""(
#[vertex]

#version 450 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormals;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

struct VertexData
{
    vec2 TexCoords;
    vec3 Normal;
    vec3 WorldPos;
    vec3 camPos;
    mat3 TBN;
};

layout (location = 2) out VertexData Output;

uniform mat4 model;
uniform mat3 normalMatrix;

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
    vec3 totalNormal = vec3(0.0);

    if (animated)
    {
        totalPosition = applyBoneTransform(vec4(aPosition, 1.0f));
        totalNormal = normalize(applyBoneTransform(vec4(aNormals, 0.0))).xyz;
    }
    else
    {
        totalPosition = vec4(aPosition, 1.0f);
        totalNormal = aNormals;
    }

    Output.WorldPos = vec3(model * totalPosition);
    Output.Normal = normalMatrix * totalNormal;
    Output.camPos = cameraPos;
    Output.TexCoords = aTexCoord;

    gl_Position = projection * view * vec4(Output.WorldPos, 1.0);

    // Tangent space matrix

    //There is other way that is more efficient that is converting the lightPos and viewPos to tangent space in the vertex shader
    //and then pass them to the fragment shader. But this way is more simple and easy to understand + for PBR is better to transform
    //the normal map to view space + im lazy to move the lights to the vertex shader

    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(totalNormal, 0.0)));

    Output.TBN = mat3(T, B, N);
}

#[fragment]

#version 450 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 EntityID;

uniform vec3 entityID;

struct VertexData
{
    vec2 TexCoords;
    vec3 Normal;
    vec3 WorldPos;
    vec3 camPos;
    mat3 TBN;
};

layout (location = 2) in VertexData VertexInput;

/*Todo: Think if we should use a struct for the material or mimmic the Material
        structs in the C++ code.
*/
struct Material
{
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
    sampler2D emissiveMap;

    vec4 color;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;

    int hasAlbedo;
    int hasNormal;
    int hasMetallic;
    int hasRoughness;
    int hasAO;
    int hasEmissive;
};

uniform Material material;

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
};

layout (std140, binding = 1) uniform RenderData
{
    Light lights[MAX_LIGHTS];
    int lightCount;
};

uniform bool showNormals;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// https://github.com/godotengine/godot/blob/da5f39889f155658cef7f7ec3cc1abb94e17d815/modules/lightmapper_rd/lm_compute.glsl#L388
float getOmniAttenuation(float distance, float inv_range, float decay) {
    float nd = distance * inv_range;
    nd *= nd;
    nd *= nd; // nd^4
    nd = max(1.0 - nd, 0.0);
    nd *= nd; // nd^2
    return nd * pow(max(distance, 0.0001), -decay);
}

void main()
{
    vec3 albedo = material.hasAlbedo * (texture(material.albedoMap, VertexInput.TexCoords).rgb * material.color.rgb) + (1 - material.hasAlbedo) * material.color.rgb;

    // Revise this type of conditional assignment (the commented one) because i think can lead to some undefined behavior in the shader!!!!!
    vec3 normal/*  = material.hasNormal * (VertexInput.TBN * (texture(material.normalMap, VertexInput.TexCoords).rgb * 2.0 - 1.0)) + (1 - material.hasNormal) * VertexInput.Normal */;
    if (material.hasNormal == 1) {
        normal = VertexInput.TBN * (texture(material.normalMap, VertexInput.TexCoords).rgb * 2.0 - 1.0);
    } else {
        normal = VertexInput.Normal;
    }
    float metallic = material.hasMetallic * (texture(material.metallicMap, VertexInput.TexCoords).b * material.metallic) + (1 - material.hasMetallic) * material.metallic;
    float roughness = material.hasRoughness * (texture(material.roughnessMap, VertexInput.TexCoords).g * material.roughness) + (1 - material.hasRoughness) * material.roughness;
    float ao = material.hasAO * (texture(material.aoMap, VertexInput.TexCoords).r * material.ao) + (1 - material.hasAO) * material.ao;
    vec3 emissive = material.hasEmissive * (texture(material.emissiveMap, VertexInput.TexCoords).rgb * material.emissive) + (1 - material.hasEmissive) * material.emissive;

    vec3 N = normalize(normal);
    vec3 V = normalize(VertexInput.camPos - VertexInput.WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < lightCount; i++)
    {
        vec3 L = vec3(0.0);

        vec3 radiance = vec3(0.0);

        if(lights[i].type == 0)
        {
            /*====Directional Light====*/

            L = normalize(-lights[i].direction);
            radiance = lights[i].color * lights[i].intensity;
        }
        else if(lights[i].type == 1)
        {
            /*====Point Light====*/

            vec3 lightDir = normalize(lights[i].position - VertexInput.WorldPos);
            float distance = length(lights[i].position - VertexInput.WorldPos);

            if(distance > lights[i].range)
                continue;

            float attenuation = getOmniAttenuation(distance, 1.0 / lights[i].range, lights[i].attenuation);
            attenuation *= max(0.0, dot(N, lightDir));

            if(attenuation <= 0.0001)
                continue;

            L = lightDir;
            radiance = lights[i].color * attenuation * lights[i].intensity;
        }
        else if(lights[i].type == 2)
        {
            /*====Spot Light====*/

        }

        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo + emissive;

    FragColor = vec4(vec3(color), 1.0);
    EntityID = vec4(entityID, 1.0f); //set the alpha to 0

    //REMOVE: This is for the first release of the engine it should be handled differently
    if(showNormals)
    {
        FragColor = vec4((N * 0.5) + 0.5, 1.0);
        return;
    }
}
)"";