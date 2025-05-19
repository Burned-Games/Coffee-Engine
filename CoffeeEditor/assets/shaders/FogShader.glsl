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

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
	vec3 color = texture(screenTexture, TexCoord).rgb;
	vec3 depth = texture(depthTexture, TexCoord).rgb;

    FragColor = vec4(vec3(color), 1.0);
}
