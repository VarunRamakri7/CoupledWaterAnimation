#version 450            

uniform float time;
uniform float particle_size = 20.0;
//uniform float particle_size = 14.0;
uniform int uMode = 0;

struct Particle
{
   vec4 pos;
   vec4 vel;
   vec4 acc;
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

out vec4 pv;
out vec4 pa;
out vec2 tex_coord; 

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

void main(void)
{
	if(uMode==0)
	{
		vec4 p = particles_in[ gl_VertexID ].pos;
		p.xy /= 0.5*vec2(VIEW_WIDTH, VIEW_HEIGHT);
		p.xy -= vec2(1.0);
		p.zw = vec2(0.0, 1.0);
	
		gl_Position = p;
		gl_PointSize = particle_size;
		pv = particles_in[ gl_VertexID ].vel;
		pa = particles_in[ gl_VertexID ].acc;
	}
	if(uMode==1)
	{
		gl_Position = quad[ gl_VertexID ]; //get clip space coords out of quad array
		tex_coord = 0.5*quad[ gl_VertexID ].xy + vec2(0.5);
	}
}