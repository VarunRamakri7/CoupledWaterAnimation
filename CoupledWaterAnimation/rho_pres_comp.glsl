#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 2500
#define PARTICLE_RADIUS 0.005f

// For calculations
#define PI 3.141592741f

layout (local_size_x = WORK_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

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

layout(std140, binding = 2) uniform BoundaryUniform
{
    vec4 upper; // Upper bounds of particle area
    vec4 lower; // Lower bounds of particle area
};

const float GAS_CONST = 4000.0f; // const for equation of state
const ivec2 texture_size = textureSize(wave_tex, 0);
const float dt = 0.00008f; // Time step

Particle wave_particle;

vec3 WaveVelocity(ivec2 uv);

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if(i >= NUM_PARTICLES) return;
    
    const float smoothing_length = smoothing_coeff * PARTICLE_RADIUS; // Smoothing length for neighbourhood

    // Compute Density (rho)
    float rho = 0.0f;

    // Iterate through all particles
    for (uint j = 0; j < NUM_PARTICLES; j++)
    {
        vec3 delta = particles[i].pos.xyz - particles[j].pos.xyz; // Get vector between current particle and particle in vicinity
        float r = length(delta); // Get length of the vector
        if (r < smoothing_length) // Check if particle is inside smoothing radius
        {
			rho += mass * 315.0f * pow(smoothing_length * smoothing_length - r * r, 3) / (64.0f * PI * pow(smoothing_length, 9)); // Use Poly6 kernal
        }
    }

	// Make wave particle
	ivec2 coord = ivec2(particles[i].pos.xz) / texture_size; // Get XZ coordinate of particle
	wave_particle.pos = particles[i].pos; // Set the same position as the current particle
	wave_particle.pos.y = lower.y - (PARTICLE_RADIUS + PARTICLE_RADIUS); // Set height of particle just below the wave surface
	wave_particle.vel = vec4(WaveVelocity(coord).xzy, 0.0f); // Calculate velocity of the wave at this point
	
	vec3 wave_acc = (wave_particle.vel.xyz - particles[i].vel.xyz) / dt;

	wave_particle.force = vec4(mass * wave_acc, 0.0f); // Force exerted by wave
	wave_particle.extras = vec4(100.0f * resting_rho, 0.00000178f, 0.0f, 0.0f); // Density, pressure, and age

	// Add force from ghost wave particle
	vec3 wave_delta = particles[i].pos.xyz - wave_particle.pos.xyz; // Vector between wave ghost particle and current particle
	float wave_r = max(0.0f, length(wave_delta));
	if(wave_r < 0.5f * smoothing_length)
	{
            rho += mass * 315.0f * pow(0.25f * smoothing_length * smoothing_length - wave_r * wave_r, 3) / (64.0f * PI * pow(0.5f * smoothing_length, 9)); // Use Poly6 kernal
	}

    particles[i].extras[0] = max(resting_rho, rho); // Assign computed value
    
    // Compute Pressure
	particles[i].extras[1] = max(GAS_CONST * (rho - resting_rho), 0.0f);
}

vec3 WaveVelocity(ivec2 uv)
{
    float h = 0.01; // Small step

    float height = texture(wave_tex, uv).r;
    float heightX = texture(wave_tex, vec2(uv.x + h, uv.y)).r;
    float heightY = texture(wave_tex, vec2(uv.x, uv.y + h)).r;
    float heightZ = texture(wave_tex, uv).r;
    float heightZX = texture(wave_tex, vec2(uv.x + h, uv.y)).r;
    float heightZY = texture(wave_tex, vec2(uv.x, uv.y + h)).r;

    vec3 velocity = vec3((heightX - height) / dt, (heightY - height) / dt, (heightZX + heightZY - 2.0 * heightZ) / (dt * dt));
	return velocity;
}