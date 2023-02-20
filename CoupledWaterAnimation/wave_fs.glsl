#version 450

layout(binding = 0) uniform sampler2D wave_tex;
layout(binding = 1) uniform samplerCube skybox_tex;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

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

vec4 reflection();

void main(void)
{
	//vec4 v = texture(wave_tex, inData.tex_coord);
	//v.x = smoothstep(-0.01, 0.01, v.x);
	
	//fragcolor = vec4(mix(wave_col, color0, v.x));

	fragcolor = reflection();
}

vec4 reflection()
{
    vec3 I = normalize(inData.pw - eye_w.xyz);
    vec3 R = reflect(I, normalize(inData.nw));
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}