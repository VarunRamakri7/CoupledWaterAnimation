#version 430            

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 M;
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

struct Particle
{
   vec4 pos;
   vec4 vel; //vel.w = particle age
};

//SSBO holds array of particle structs.
//Be sure to check the std430 rules for alignment
layout (std430, binding = 13) buffer PARTICLES 
{
	Particle particles[];
};

out float age;

void main(void)
{
   //glVertexID is the SSBO array index
   const int ix = gl_VertexID;
   vec4 pos = particles[ix].pos;
   vec4 vel = particles[ix].vel;
   age = vel.w;

   gl_Position = PV*M*pos;
   gl_PointSize = 2.75+age/2000.0;
   
}
