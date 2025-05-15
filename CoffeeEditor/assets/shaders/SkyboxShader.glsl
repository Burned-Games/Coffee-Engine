#[vertex]

#version 450 core

layout (location = 0) in vec3 aPosition;

layout (std140, binding = 0) uniform camera
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
};

out vec3 TexCoord;

void main()
{
    mat4 view2 = mat4(mat3(view));

    TexCoord = aPosition;
    vec4 pos = projection * view2 * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}

#[fragment]

#version 450 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 EntityID;

in vec3 TexCoord;

uniform samplerCube skybox;
uniform float exposure;

void main()
{
    vec3 color = texture(skybox, TexCoord).rgb * exposure;
    FragColor = vec4(color, 1.0);
    EntityID = vec4(1.0, 1.0, 1.0, 1.0);
}