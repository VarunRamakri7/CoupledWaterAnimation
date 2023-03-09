#version 440

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

//layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec4 tex_coord_attrib;

out VertexData
{
	vec3 particle_pos;
	vec2 tex_coord;
	float depth;
} outData;

const float near = 0.1f; // Near plane distance
const float far = 100.0f; // Far plane distance

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
								vec4(-1.0, -1.0, 0.0, 1.0), 
								vec4( 1.0, 1.0, 0.0, 1.0), 
								vec4( 1.0, -1.0, 0.0, 1.0));

void main ()
{
	// Render particles or depth or calculate normals
	if(pass == 0 || pass == 2 || pass == 3)
	{
		outData.tex_coord = tex_coord_attrib.xy;

		vec4 eye_pos = PV * M * vec4(pos_attrib, 1.0f);
		gl_Position = eye_pos;

		outData.particle_pos = vec3(M * vec4(pos_attrib, 1.0));
		outData.depth = eye_pos.z / eye_pos.w; // Calculate eye-space depth
	}

	// Render full-screen quad
	if(pass == 1)
	{
		gl_Position = quad[gl_VertexID]; //get clip space coords out of quad array
		outData.tex_coord = 0.5f * (quad[gl_VertexID].xy + vec2(1.0f)); 
	}
}