#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 2500
#define PARTICLE_RADIUS 0.005f

// For calculations
#define PI 3.141592741f

layout (local_size_x = WORK_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

//layout(rgba32f, binding = 2) readonly restrict uniform image2D uOutputImage; //wave at time t

layout(binding = 0) uniform sampler2D wave_tex;

struct Particle
{
	vec4 pos;
	vec4 vel;
	vec4 force;
	vec4 extras; // 0 - rho, 1 - pressure, 2 - age
};

struct Wave
{
	vec4 pos;
	vec4 tex_coords; // XY - UV, ZW - Grid coordinate
	vec4 normals;
};

layout(std430, binding = 0) buffer PARTICLES
{
	Particle particles[];
};

layout(std430, binding = 1) buffer WAVE
{
	Wave waves[];
};

layout(std140, binding = 1) uniform ConstantsUniform
{
	float mass; // Particle Mass
	float smoothing_coeff; // Smoothing length coefficient for neighborhood
	float visc; // Fluid viscosity
	float resting_rho; // Resting density
};

const vec3 G = vec3(0.0f, -9806.65f, 0.0f); // Gravity force
Particle wave_particle;

void main()
{
	uint i = gl_GlobalInvocationID.x;
	if(i >= NUM_PARTICLES) return;
	
	// Make wave particle
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	wave_particle.pos = particles[i].pos; // Set the same position as the current particle
	wave_particle.pos.y -= 2.0f * PARTICLE_RADIUS; // Set ghost particle height jsut below surface
	wave_particle.vel = vec4(0.0f, 1.0f, 0.0f, 0.0f); // Constant upward velocity
	wave_particle.force = vec4(0.0f, -100.0f * textureLod(wave_tex, gid, 0.0f).y, 0.0f, 0.0f); // Constant downward force
	wave_particle.extras = vec4(100 * resting_rho, 0.00000178f, 0.0f, 0.0f); // Density, pressure, and age

	const float smoothing_length = smoothing_coeff * PARTICLE_RADIUS; // Smoothing length for neighbourhood
	const float spiky = -45.0f / (PI * pow(smoothing_length, 6)); // Spiky kernal
	const float laplacian = -spiky; // Laplacian kernel

	// Compute all forces
	vec3 pres_force = vec3(0.0f);
	vec3 visc_force = vec3(0.0f);
	
	for (uint j = 0; j < NUM_PARTICLES; j++)
	{
		if (i == j)
		{
			continue;
		}

		vec3 delta = particles[i].pos.xyz - particles[j].pos.xyz; // Get vector between current particle and particle in vicinity
		float r = length(delta); // Get length of the vector
		if (r < smoothing_length) // Check if particle is inside smoothing radius
		{
			pres_force -= mass * (particles[i].extras[1] + particles[j].extras[1]) / (2.0f * particles[j].extras[0]) * spiky * pow(smoothing_length - r, 2) * normalize(delta); // Gradient of Spiky Kernel
			visc_force += mass * (particles[j].vel.xyz - particles[i].vel.xyz) / particles[j].extras[0] * laplacian * (smoothing_length - r); // Laplacian of viscosity kernel
		}
	}
	visc_force *= visc;
	
	// Add force from ghost wave particle
	vec3 wave_delta = particles[i].pos.xyz - wave_particle.pos.xyz; // Vector between wave ghost particle and current particle
	float wave_r = length(wave_delta);
	pres_force -= mass * (particles[i].extras[1] + wave_particle.extras[1]) / (2.0f * wave_particle.extras[0]) * spiky * pow(smoothing_length - wave_r, 2) * normalize(wave_delta); // Gradient of Spiky Kernel
	visc_force += mass * (wave_particle.vel.xyz - particles[i].vel.xyz) / wave_particle.extras[0] * laplacian * (smoothing_length - wave_r); // Laplacian of viscosity kernel

	vec3 grav_force = particles[i].extras[0] * G;
	particles[i].force.xyz = pres_force + visc_force + grav_force;
}