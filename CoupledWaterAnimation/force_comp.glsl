#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 10000
#define PARTICLE_RADIUS 0.005f

// For calculations
#define PI 3.141592741f

layout (local_size_x = WORK_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 1) uniform float time;

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

layout(std140, binding = 2) uniform BoundaryUniform
{
    vec4 upper; // XYZ - Upper bounds, W - Foam threshold
    vec4 lower; // XYZ - Lower bounds, W - Density coefficient
};

layout(std140, binding = 3) uniform WaveUniforms
{
	vec4 attributes; // Lambda, Attenuation, Beta
};

const vec3 G = vec3(0.0f, -9806.65f, 0.0f); // Gravity force
const float smoothing_length = smoothing_coeff * PARTICLE_RADIUS; // Smoothing length for neighbourhood
const ivec2 texture_size = textureSize(wave_tex, 0);
const float dt = 0.00005f; // Time step

Particle wave_particle;

vec3 WaveVelocity(vec2 uv);
vec3 WaveNormal(vec2 uv);

void main()
{
	uint i = gl_GlobalInvocationID.x;
	if(i >= NUM_PARTICLES) return;

	// Kernels
	const float spiky = -20.0f / (PI * pow(smoothing_length, 6)); // Spiky kernal
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

	vec2 coord = particles[i].pos.xz * 2.0f; // Get XZ coordinate of particle
	float height = texture(wave_tex, coord).r; // Sample height of wave

	visc_force *= visc;

	// Calculate torque of particles
	vec3 torque = cross(particles[i].pos.xyz, particles[i].force.xyz);
	torque *= 0.25f; // Rotational coefficient

	// Drag force from the wave on the particles
	vec3 rel_vel = particles[i].vel.xyz - WaveVelocity(coord); // Relative velocity
	vec3 drag_force = -0.25f * rel_vel;

	vec3 wave_force = -height * WaveNormal(coord) * 0.5f; // Approximate force from the wave in the opposite direction

	// Combine all forces
	vec3 grav_force = particles[i].extras[0] * G;
	particles[i].force.xyz = pres_force + visc_force + grav_force + torque + drag_force + wave_force;
}

vec3 WaveVelocity(vec2 uv)
{
    const float h = 0.01f; // Small step

	float height = texture(wave_tex, uv).r;

	float heightX = texture(wave_tex, vec2(uv.x + h, uv.y)).r;
	float heightY = texture(wave_tex, vec2(uv.x, uv.y + h)).r;

	vec3 velocity = vec3((heightX - height) / dt, (heightY - height) / dt, (heightX - heightY) / h);
	return velocity;
}

// Normal to the wave at the given coordinate
vec3 WaveNormal(vec2 uv)
{
	float height = texture(wave_tex, uv).r;

    vec3 dx = vec3(1.0, 0.0, texture(wave_tex, uv + vec2(1.0, 0.0)).r - height);
    vec3 dy = vec3(0.0, 1.0, texture(wave_tex, uv + vec2(0.0, 1.0)).r - height);

    return cross(dy, dx);
}