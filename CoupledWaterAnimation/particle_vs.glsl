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

out VertexData
{
	vec3 particle_pos;
	vec2 tex_coord;
} outData;

void main ()
{
	outData.tex_coord = tex_coord_attrib.xy;

    gl_Position = PV * M * vec4(pos_attrib, 1.0f);
	outData.particle_pos = vec3(M * vec4(pos_attrib, 1.0));
}