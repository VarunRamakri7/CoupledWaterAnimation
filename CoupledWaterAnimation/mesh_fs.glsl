#version 440

layout(binding = 1) uniform sampler2D diffuse_tex;

layout(location = 1) uniform float time;

in vec2 tex_coord; 

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	fragcolor = texture(diffuse_tex, tex_coord);
}