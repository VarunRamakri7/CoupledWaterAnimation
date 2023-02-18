#version 450

layout(binding = 0) uniform sampler2D wave_tex;
layout(location = 1) uniform float time;

in VertexData
{
   vec2 tex_coord;
   vec3 pw;
   vec3 nw;
   vec3 color;
} inData;

out vec4 fragcolor;

const vec4 wave_col = vec4(0.0f, 0.5f, 1.0f, 1.0f); // Azure
const vec4 color0 = vec4(0.6f, 0.8f, 1.0f, 1.0f); // Baby-blue

void main(void)
{
	vec4 v = texture(wave_tex, inData.tex_coord);
	v.x = smoothstep(-0.01, 0.01, v.x);
	
	fragcolor = vec4(mix(wave_col, color0, v.x));
}