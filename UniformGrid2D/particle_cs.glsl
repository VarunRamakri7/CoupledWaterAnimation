#version 430
layout(local_size_x = 1024) in;
          
layout(location = 0) uniform int num_particles;
layout(location = 1) uniform float time;

const float PI = 3.1415926535;

struct Particle
{
   vec4 pos;
   vec4 vel; //vel.w = particle age
};

//SSBO holds array of particle structs.
//Be sure to check the std430 rules for alignment
layout (std430, binding = 0) buffer PARTICLES 
{
	Particle particles[];
};

//Basic velocity field
vec3 v0(vec3 p);

//pseudorandom number
float rand(vec2 co);
vec3 rand3(vec3 p)
{
	vec3 q = vec3(
		dot(p, vec3(127.1, 311.7, 74.7)),
		dot(p, vec3(269.5, 183.3, 246.1)),
		dot(p, vec3(113.5, 271.9, 124.6))
		);

	return fract(sin(q) * 43758.5453123);
}

float max3(vec3 a)
{
   return max(a.x, max(a.y, a.z));
}

void main(void)
{
   //glVertexID is the SSBO array index
   const int ix = int(gl_GlobalInvocationID.x);
   vec4 pos = particles[ix].pos;
   vec4 vel = particles[ix].vel;

   //Set new velocity
   vel.y -= 0.01;

   float age = vel.w-1.0;
   //Integrate to update position
   pos.xyz += 0.008*vel.xyz;

   //Reinitialize particles as needed
   if(age <= 0.0 || max3(abs(pos.xyz)) > 1.0)
   {
      vec2 seed = vec2(0.0001*float(ix), time/100.0); //seed for the random number generator
      age = 400.0 + 200.0*rand(seed);
      //Pseudorandom position
      pos.xyz = 0.1*(rand3(pos.xyz*seed.x)-vec3(0.5)); 
      vel.xyz = 5.0*rand3(pos.xyz)*vec3(pos.x, 0.3, pos.y);
   }

   //write back to ssbo
   particles[ix].pos = vec4(pos.xyz, 1.0);
   particles[ix].vel = vec4(vel.xyz, age);
}

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}
