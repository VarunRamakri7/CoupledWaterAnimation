#version 440

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec4 tex_coord_attrib;

out vec3 particle_pos;

void main ()
{
	//vec4 pos = vec4(pos_attrib.xyz, 1.0);

	//float height = textureLod(wave_tex, tex_coord_attrib.xy, 0.0).r;
	//pos.y = 50.0f * height;

	//gl_Position = PV * M * pos;
	//particle_pos = vec3(M * pos);

    gl_Position = PV * M * vec4(pos_attrib, 1.0f);
	particle_pos = vec3(M * vec4(pos_attrib, 1.0));
}