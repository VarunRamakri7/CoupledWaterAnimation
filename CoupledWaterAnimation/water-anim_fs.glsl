#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

in vec3 particle_pos;

//in VertexData
//{
//	vec2 tex_coord;
//	vec3 pw;       //world-space vertex position
//	vec3 nw;   //world-space normal vector
//	vec3 color;
//} inData;

out vec4 frag_color;

const vec3 particle_col = vec3(0.1f, 0.65f, 0.8f); // Blue color
const vec3 light_col = vec3(1.0f, 0.8f, 0.65f); // Warm light_col
const vec3 light_pos = vec3(0.0f, 0.5f, 0.0f);
const float intensity = 10.0f;

void main ()
{
	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.35f));
    if (r >= 0.35) discard;

    frag_color = vec4(particle_col, 1.0f);
}