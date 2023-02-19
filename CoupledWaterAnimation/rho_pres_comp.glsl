#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 10000
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
    vec4 upper; // XYZ - Upper bounds, W - Foam threshold
    vec4 lower; // XYZ - Lower bounds, W - Density coefficient
};

const float GAS_CONST = 4000.0f; // const for equation of state
const ivec2 texture_size = textureSize(wave_tex, 0);
const float dt = 0.00005f; // Time step

Particle wave_particle;

float WaveSteepness(vec2 uv);

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

    //particles[i].extras[0] = max(resting_rho, rho); // Assign computed value
	//particles[i].extras[1] = max(GAS_CONST * (rho - resting_rho), 0.0f); // Compute Pressure

    float pressure = max(GAS_CONST * (rho - resting_rho), 0.0f); // Compute Pressure

    vec2 coord = 2.0f * particles[i].pos.xz;
    float height = texture(wave_tex, coord).r;

    float wave_force = height * rho; // Approximate force from wave
    pressure += wave_force;
    rho += wave_force / (GAS_CONST * PARTICLE_RADIUS);

    //float wave_velocity = sqrt(9.81f * height);
    //float vel = length(particles[i].vel.xyz - wave_velocity * vec3(1.0, 0.0, 0.0));
    
    // Calculate wave force with Bernouli's Equation
    //float p1 = 0.5f * resting_rho * PARTICLE_RADIUS * PARTICLE_RADIUS * height * height;
    //float p2 = 0.25f * resting_rho * PARTICLE_RADIUS * PARTICLE_RADIUS * vel * vel;
    //float wave_force = 0.45f * (p1 + p2);
    
    // Update density and pressure
    //rho += wave_force / (GAS_CONST * PARTICLE_RADIUS);
    //pressure += wave_force;
    
    // Check if breaking wave threshold is exceeded
    float steepness = WaveSteepness(coord);
    if (steepness > 0.01f)
    {
        // Reduce smoothing length and increase pressure
        const float breaking_smoothing_length = 0.5f * smoothing_length;
        for (uint j = 0; j < NUM_PARTICLES; j++)
        {
            vec3 delta = particles[i].pos.xyz - particles[j].pos.xyz; // Get vector between current particle and particle in vicinity
            float r = length(delta); // Get length of the vector
            if (r < breaking_smoothing_length) // Check if particle is inside breaking smoothing radius
            {
                rho += mass * 315.0f * pow(breaking_smoothing_length * breaking_smoothing_length - r * r, 3) / (64.0f * PI * pow(breaking_smoothing_length, 9)); // Use Poly6 kernal
                pressure += 0.5f * (0.01f - steepness);
            }
        }
    }

    particles[i].extras[0] = max(resting_rho, rho); // Assign computed density
	particles[i].extras[1] = pressure; // Assign computed pressure
}

float WaveSteepness(vec2 uv)
{
    float h = texture(wave_tex, uv).r;
    
    // Sample wave height at neighboring pixels
    float hL = texture(wave_tex, vec2(uv.x - 1.0f / texture_size.x, uv.y)).r;
    float hR = texture(wave_tex, vec2(uv.x + 1.0f / texture_size.x, uv.y)).r;
    float hB = texture(wave_tex, vec2(uv.x, uv.y - 1.0f / texture_size.x)).r;
    float hT = texture(wave_tex, vec2(uv.x, uv.y + 1.0f / texture_size.x)).r;
    
    // Calculate slope in x and y direction
    float slopeX = (hL - 2.0*h + hR) / pow(1.0f / texture_size.x, 2);
    float slopeY = (hB - 2.0*h + hT) / pow(1.0f / texture_size.x, 2);
    
    // Calculate steepness using the slope    
    return sqrt(pow(slopeX, 2) + pow(slopeY, 2));
}