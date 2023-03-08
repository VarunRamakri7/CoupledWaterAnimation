#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(binding = 0) uniform sampler2D wave_tex;
layout(binding = 1) uniform samplerCube skybox_tex;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV; // Projection x View Matrix
   vec4 eye_w; // Camera eye in world-space
};

in VertexData
{
	vec3 particle_pos;
	vec2 tex_coord;
} inData;

out vec4 frag_color;

const vec4 particle_col = vec4(0.4f, 0.7f, 1.0f, 1.0f); // Light blue
const vec4 foam_col = vec4(1.0f); // White

void main ()
{
	//float height = textureLod(wave_tex, particle_pos.xz * 2.0f, 0.0f).r;
	//if (particle_pos.y < 0.0f) discard; // Ignore particles that are below origin

	// Make circular particles
    float r = length(gl_PointCoord - vec2(0.5f));
    if (r >= 0.5f) discard;
	
	// Add specular highlight
	float spec = pow(max(dot(normalize(inData.particle_pos), normalize(vec3(0.0f, 1.0f, 0.0f))), 0.0f), 16.0f);
	spec = pow(spec, 2);
	float a = exp(-12.0f * r) + 0.1f * exp(-2.0f * r); // Change transparency based on distance from center

	// Change particle color depending on height
	frag_color = mix(particle_col, foam_col, 2.0f * inData.particle_pos.y);
	frag_color.rgb += spec; // Add specular highlight
	frag_color.a = a;
}