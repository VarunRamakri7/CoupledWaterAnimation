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

const vec3 wave_col = vec3(1.0f, 0.0f, 0.0f); // Red
const vec4 color0 = vec4(1.0f, 0.95f, 0.85, 1.0f); // Whitish-Red
const vec4 color1 = vec4(0.9f, 0.6f, 0.0f, 1.0f); // Purple

void main(void)
{
	vec4 v = texture(wave_tex, inData.tex_coord);
	v.x = smoothstep(-0.01, 0.01, v.x);
	
	fragcolor = vec4(mix(color1, color0, v.x));
}