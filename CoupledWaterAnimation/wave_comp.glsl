#version 440

#define WAVE_RES 512
#define PARTICLE_RADIUS 0.005f

layout(local_size_x = 32, local_size_y = 32) in;

//layout(rgba32f, binding = 0) readonly restrict uniform image2D uInputImage0; //wave at time t-1
//layout(rgba32f, binding = 1) readonly restrict uniform image2D uInputImage1; //wave at time t-2
//layout(rgba32f, binding = 2) writeonly restrict uniform image2D uOutputImage; //wave at time t

//layout (binding = 3) uniform sampler2D uInitImage;

layout(location = 3) uniform int mode;

const int MODE_INIT = 0;
const int MODE_INIT_FROM_TEXTURE = 1;
const int MODE_EVOLVE = 2;

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
    vec4 tex_coords;
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
    vec4 upper; // Upper bounds of particle area
    vec4 lower; // Lower bounds of particle area
};

layout(std140, binding = 3) uniform WaveUniforms
{
   float Lambda;
   float Atten;
   float Beta;   
};

const float dt = 0.0001f; // Time step

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    waves[gid.x].pos = vec4(Lambda, Atten, Beta, 0.0f);
}