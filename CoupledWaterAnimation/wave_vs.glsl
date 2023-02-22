#version 450

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 0) in vec4 pos_attrib;
layout(location = 1) in vec4 tex_coord_attrib;
layout(location = 2) in vec4 normal_attrib;
layout(location = 3) in vec3 color_attrib;
//layout(location = 4) in mat4 offset_attrib;

out VertexData
{
	vec2 tex_coord;
	vec3 pw; // world-space vertex position
	vec3 nw; // world-space normal vector
	vec3 color;
} outData; 

void main(void)
{
	vec4 pos = vec4(pos_attrib.xyz, 1.0);

	float height = textureLod(wave_tex, tex_coord_attrib.xy, 0.0).r;
	pos.y = 50.0f * height;

	gl_Position = PV * M * pos;

	outData.tex_coord = tex_coord_attrib.xy;
	outData.pw = vec3(M * vec4(pos_attrib.xyz, 1.0));
	outData.nw = -normal_attrib.xyz; // Invert normals
	outData.color = color_attrib;
}