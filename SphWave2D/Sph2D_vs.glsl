#version 450            

uniform float time;
uniform float particle_size = 8.0;
layout(binding = 0) uniform sampler1D wave_tex;

out vec2 tex_coord; 

struct Particle
{
   vec4 pos;
   vec4 vel;
   vec4 force;
};

layout (std430, binding = 0) readonly restrict buffer PARTICLES_IN 
{
	Particle particles_in[];
};

const float PARTICLE_RADIUS = 0.025;
const float PARTICLE_DIAM = 2.0*PARTICLE_RADIUS;
const int WIDTH = 32;
const int HEIGHT = 32;
const float VIEW_WIDTH = 2.0*4.8;//3*WIDTH*PARTICLE_DIAM;
const float VIEW_HEIGHT = 2.0*4.8;//3*HEIGHT*PARTICLE_DIAM;

out vec4 v;
out vec4 pos; //clip space pos

vec4 discard_pos = vec4(0.0);

void main(void)
{
	pos = particles_in[ gl_VertexID ].pos;
	if(pos.w == 0.0)
	{
		gl_Position = discard_pos;
		return;
	}

	pos = vec4(particles_in[ gl_VertexID ].pos.xy, 0.0, 1.0);

	vec2 tex_coord = pos.xy/vec2(VIEW_WIDTH, VIEW_HEIGHT);
	vec4 wave = textureLod(wave_tex, tex_coord.x, 0);

	pos.xy /= 0.5*vec2(VIEW_WIDTH, VIEW_HEIGHT);
	pos.xy -= vec2(1.0);
	
	gl_Position = pos;
	gl_PointSize = particle_size;
	v = particles_in[ gl_VertexID ].vel;
	pos.w = particles_in[ gl_VertexID ].pos.w;
}