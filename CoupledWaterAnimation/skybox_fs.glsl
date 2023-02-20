#version 450

layout(binding = 0) uniform samplerCube skybox_tex;

out vec4 fragcolor;

in vec3 tex_coord;

uniform samplerCube skybox;

void main()
{    
    fragcolor = texture(skybox_tex, tex_coord);
}