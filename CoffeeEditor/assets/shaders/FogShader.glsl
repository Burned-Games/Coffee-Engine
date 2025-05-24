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
uniform float near;
uniform float far;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec3 color = texture(colorTexture, TexCoord).rgb;
    float depth = texture(depthTexture, TexCoord).r;

    float linearDepth = LinearizeDepth(depth);
    float fog = clamp((linearDepth - near) / (far - near), 0.0, 1.0);
    color = mix(color, vec3(0.85, 0.74, 0.52), fog);

    FragColor = vec4(color, 1.0);
}