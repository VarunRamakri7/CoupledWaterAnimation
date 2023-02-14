#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(binding = 0) uniform sampler2D wave_tex;

in vec3 particle_pos;
in vec2 tex_coord;

out vec4 frag_color;

const vec4 particle_col = vec4(0.1f, 0.65f, 0.8f, 1.0f); // Blue color
const vec4 color0 = vec4(0.85f, 0.95f, 1.00, 1.0f); // Whitish-Blue
const vec4 color1 = vec4(0.9f, 0.6f, 0.0f, 1.0f); // Purple

void main ()
{
	if (particle_pos.y < 0.0f) discard; // Ignore particles that are below surface

	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.35f));
    if (r >= 0.35) discard;

    frag_color = particle_col;

	//vec4 v = texture(wave_tex, tex_coord);
	//v.x = smoothstep(-0.01, 0.01, v.x);
	
	//frag_color = vec4(mix(particle_col, color0, v.x));
}