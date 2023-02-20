#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(binding = 0) uniform sampler2D wave_tex;

in vec3 particle_pos;
in vec2 tex_coord;

out vec4 frag_color;

//const vec4 particle_col = vec4(0.8f, 0.97f, 1.0f, 1.0f); // Light cyan
const vec4 particle_col = vec4(0.2f, 0.5f, 1.0f, 1.0f); // Light blue
const vec4 foam_col = vec4(1.0f); // White

void main ()
{
	if (particle_pos.y < 0.0f) discard; // Ignore particles that are below surface

	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.45f));
    if (r >= 0.45f) discard;
	
	//frag_color = vec4(mix(particle_col, foam_col, 2.0f * particle_pos.y)); // Change particle color depending on height

	// Add specular highlight
	float spec = pow(max(dot(normalize(particle_pos), normalize(vec3(0.0f, 1.0f, 0.0f))), 0.0f), 16.0f);

	// Change particle color depending on height
	frag_color = mix(particle_col + vec4(0.2f, 0.2f, 0.2f, 0.0f), foam_col, 2.0f * particle_pos.y);
	frag_color.a = 1.0f;
	frag_color.rgb += spec; // Add specular highlight
}