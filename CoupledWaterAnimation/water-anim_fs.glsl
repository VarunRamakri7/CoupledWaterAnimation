#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

in vec3 particle_pos;

out vec4 frag_color;

const vec3 particle_col = vec3(0.1f, 0.65f, 0.8f); // Blue color

void main ()
{
	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.35f));
    if (r >= 0.35) discard;

    frag_color = vec4(particle_col, 1.0f);
}