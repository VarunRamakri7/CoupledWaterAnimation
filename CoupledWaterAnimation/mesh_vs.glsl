#version 440

layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
	mat4 PV;
	mat4 P;
	mat4 V;
	vec4 eye_w; // Camera eye in world-space
};

in vec3 pos_attrib; //this variable holds the position of mesh vertices
in vec2 tex_coord_attrib;
in vec3 normal_attrib;  

out vec2 tex_coord; 

void main(void)
{
	vec4 pos = vec4(pos_attrib.xyz, 1.0);

	float height = textureLod(wave_tex, pos_attrib.xy, 0.0).r;
	pos.y += 0.2f * height;

	gl_Position = PV * M * pos; //transform vertices and send result into pipeline
	tex_coord = tex_coord_attrib; //send tex_coord to fragment shader
}