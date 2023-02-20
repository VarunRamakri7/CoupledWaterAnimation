#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 10000
#define PARTICLE_RADIUS 0.005f

// For calculations
#define DAMPING 0.3f // Boundary epsilon

layout (local_size_x = WORK_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D wave_tex;

layout(location = 0) uniform mat4 M;

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

layout(std140, binding = 2) uniform BoundaryUniform
{
    vec4 upper; // XYZ - Upper bounds, W - Foam threshold
    vec4 lower; // XYZ - Lower bounds, W - Density coefficient
};

const float dt = 0.00005f; // Time step

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if(i >= NUM_PARTICLES) return;

    // Integrate all components
    vec3 acceleration = particles[i].force.xyz / particles[i].extras[0];
    vec3 new_vel = particles[i].vel.xyz + dt * acceleration;
    vec3 new_pos = particles[i].pos.xyz + dt * new_vel;

    new_vel *= 1.0f - DAMPING * dt; // Damp particle velocity

    // Make particle behave like foam
    if(length(new_vel) > 25.0f)
    {
        // Decrease and damp attributes to emulate foam-like behaviour
        particles[i].force *= 0.5f;
        particles[i].extras[0] *= 0.1f;
        particles[i].extras[1] *= 0.25f;
        new_vel *= 0.1f;
    }

    // Boundary conditions
    if (new_pos.x < lower.x)
    {
        new_pos.x = lower.x;
        new_vel.x *= -DAMPING;
    }
    else if (new_pos.x > upper.x)
    {
        new_pos.x = upper.x;
        new_vel.x *= -DAMPING;
    }
    
    if (new_pos.y < lower.y)
    {
        new_pos.y = lower.y;
        new_vel.y *= -DAMPING;
    }
    else if (new_pos.y > upper.y)
    {
        new_pos.y = upper.y;
        new_vel.y *= -DAMPING;
    }
    
    if (new_pos.z < lower.z)
    {
        new_pos.z = lower.z;
        new_vel.z *= -DAMPING;
    }
    else if (new_pos.z > upper.z)
    {
        new_pos.z = upper.z;
        new_vel.z *= -DAMPING;
    }

    // Check if particle is below surface
    if (new_pos.y < 0.0f)
    {
        new_pos.y = texture(wave_tex, new_pos.xz * 2.0f).r - PARTICLE_RADIUS; // Place particle on wave
    }

    // Assign calculated values
    particles[i].vel.xyz = new_vel;
    particles[i].pos.xyz = new_pos;
}