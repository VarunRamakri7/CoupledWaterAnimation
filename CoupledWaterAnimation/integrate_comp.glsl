#version 450

#define WORK_GROUP_SIZE 1024
#define NUM_PARTICLES 20480
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

layout(std430, binding = 0) buffer PARTICLES
{
    Particle particles[];
};

layout(std140, binding = 0) uniform SceneUniforms
{
	mat4 PV;
	mat4 P;
	mat4 V;
	vec4 eye_w; // Camera eye in world-space
};

layout(std140, binding = 2) uniform BoundaryUniform
{
    vec4 upper; // XYZ - Upper bounds, W - Foam threshold
    vec4 lower; // XYZ - Lower bounds, W - Density coefficient
    vec4 mesh_aabb_min; // Mesh min bounding box
    vec4 mesh_aabb_max; // Mesh max bounding box
};

const float dt = 0.00005f; // Time step

void CheckMeshCollision(inout vec3 pos, inout vec3 vel);
void CheckBoundary(inout vec3 pos, inout vec3 vel);

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
    
    // Check if particle is below surface
    float tex_height = texture(wave_tex, new_pos.xz * 2.0f).r;
    if (new_pos.y < tex_height)
    {
        new_pos.y = tex_height - PARTICLE_RADIUS; // Place particle below wave
    }

    // Check collisions
    CheckMeshCollision(new_pos, new_vel);
    CheckBoundary(new_pos, new_vel);

    // Assign calculated values
    particles[i].vel.xyz = new_vel;
    particles[i].pos.xyz = new_pos;
}

// Check collision with mesh
void CheckMeshCollision(inout vec3 pos, inout vec3 vel)
{
    // Check collision with mesh
    vec4 mesh_box_max = PV * M * mesh_aabb_max; // World space bounding box max
    vec4 mesh_box_min = PV * M * mesh_aabb_min; // World space bounding box min
    if (pos.x > mesh_box_min.x)
    {
        pos.x = mesh_box_min.x;
        vel.x *= -DAMPING;
    }
    else if (pos.x < mesh_box_max.x)
    {
        pos.x = mesh_box_max.x;
        vel.x *= -DAMPING;
    }
    
    if (pos.y < mesh_box_max.y)
    {
        pos.y = mesh_box_max.y;
        vel.y *= -DAMPING;
    }
    else if (pos.y > mesh_box_min.y)
    {
        pos.y = mesh_box_min.y;
        vel.y *= -DAMPING;
    }
    
    if (pos.z < mesh_box_max.z)
    {
        pos.z = mesh_box_max.z;
        vel.z *= -DAMPING;
    }
    else if (pos.z > mesh_box_min.z)
    {
        pos.z = mesh_box_min.z;
        vel.z *= -DAMPING;
    }
}

// Check Boundary conditions
void CheckBoundary(inout vec3 pos, inout vec3 vel)
{
    if (pos.x < lower.x)
    {
        pos.x = lower.x;
        vel.x *= -DAMPING;
    }
    else if (pos.x > upper.x)
    {
        pos.x = upper.x;
        vel.x *= -DAMPING;
    }
    
    if (pos.y < lower.y)
    {
        pos.y = lower.y;
        vel.y *= -DAMPING;
    }
    else if (pos.y > upper.y)
    {
        pos.y = upper.y;
        vel.y *= -DAMPING;
    }
    
    if (pos.z < lower.z)
    {
        pos.z = lower.z;
        vel.z *= -DAMPING;
    }
    else if (pos.z > upper.z)
    {
        pos.z = upper.z;
        vel.z *= -DAMPING;
    }

    if(attributes.w > 0.0f && attributes.w < 1.0f)
    {
        if(pos.y > upper.y + 0.1f)
        {
            pos.y = upper.y + 0.1f;
            vel.y *= -DAMPING;
        }
    }
}