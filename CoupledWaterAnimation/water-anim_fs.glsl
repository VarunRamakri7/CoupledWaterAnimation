#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

in VertexData
{
	//vec4 particle_pos;
	vec2 tex_coord;
	vec3 pw;       //world-space vertex position
	vec3 nw;   //world-space normal vector
	vec3 color;
} inData;

out vec4 frag_color;

const vec3 particle_col = vec3(0.1f, 0.65f, 0.8f); // Blue color
const vec3 wave_col = vec3(1.0f, 0.0f, 0.0f); // Red color
const vec3 light_col = vec3(1.0f, 0.8f, 0.65f); // Warm light_col
const vec3 light_pos = vec3(0.0f, 0.5f, 0.0f);
const float intensity = 10.0f;

void main ()
{
	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.35f));
    if (r >= 0.35) discard;

    // Calculate diffuse lighting
    //vec3 normal = normalize(vec3(1.0f - pow(gl_FragCoord.x, 2) - pow(gl_FragCoord.y, 2))); // Get normal of particle
    //vec3 light_dir = normalize(light_pos - inData.particle_pos.xyz);
    //vec3 diffuse = max(dot(normal, light_dir), 0.0f) * light_col;
    //diffuse *= intensity;

    //frag_color = vec4(particle_col * diffuse, 1.0f);
    //frag_color = vec4(particle_col, 1.0f);

    frag_color = vec4(pass == 0 ? particle_col : wave_col, 1.0f); // Draw color according to pass
}