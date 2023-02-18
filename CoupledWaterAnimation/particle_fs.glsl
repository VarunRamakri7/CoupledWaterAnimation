#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(binding = 0) uniform sampler2D wave_tex;

in vec3 particle_pos;
in vec2 tex_coord;

out vec4 frag_color;

const vec4 particle_col = vec4(0.1f, 0.7f, 1.0f, 1.0f); // Blue
const vec4 color0 = vec4(0.7f, 0.85f, 1.0f, 1.0f); // Whitish-Blue

void main ()
{
	//if (particle_pos.y < 0.0f) discard; // Ignore particles that are below surface

	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.35f));
    if (r >= 0.35) discard;

    //frag_color = particle_col;
	
	frag_color = vec4(mix(particle_col, color0, 2.0f * particle_pos.y)); // Change particle color depending on height
}