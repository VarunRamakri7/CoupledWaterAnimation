#version 450

layout(local_size_x = 1024) in;

struct Particle
{
   vec4 pos;
   vec4 vel;
};

const float mass = 1.0;
const float radius = 0.01;
const float h = 4.0*radius;
const float viscosity = 1.0;
const float PI = 3.14159265;
float sigma2 = 40.0/(7.0*PI*h*h); //For 2D
float sigma3 = 8.0/(PI*h*h*h); //For 3D
float sigma = sigma2;
const float dt = 0.01;
const vec4 g = vec4(0.0, -0.1, 0.0, 0.0);

layout (std430, binding = 0) readonly restrict buffer IN 
{
	Particle particles_in[];
};

layout (std430, binding = 1) writeonly restrict buffer OUT 
{
	Particle particles_out[];
};

layout(location = 0) uniform int uMode; 
layout(location = 1) uniform int num_particles;

const int MODE_INIT = 0;
const int MODE_EVOLVE = 1;

void InitParticle(int ix);
void EvolveParticle(int ix);

void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= num_particles) return;

	switch(uMode)
	{
		case MODE_INIT:
			InitParticle(gid);
		break;

		case MODE_EVOLVE:
			EvolveParticle(gid);
		break;
	}
}

vec4 init_grid(int ix);



void InitParticle(int ix)
{
	//particles_out[ix].pos = vec4(0.0, 0.0, 0.0, 1.0);
	particles_out[ix].pos = init_grid(ix);
	particles_out[ix].vel = vec4(0.0);
}

void EvolveParticle(int ix)
{
	//do nothing
	//Particle p_out = particles_in[ix];
	//particles_out[ix] = p_out;

	//gravity
	
	Particle p_out = particles_in[ix];
	p_out.vel += g*dt;
	p_out.pos += p_out.vel*dt;
	particles_out[ix] = p_out;
}

vec4 init_grid(int ix)
{
	int rows = int(sqrt(num_particles));
	ivec2 ij = ivec2(ix/rows, ix%rows);
	vec2 p = 0.03*(ij-ivec2(rows/2));
	return vec4(p, 0.0, 1.0);
}

float W(vec3 r, float h)
{
	float q = length(r)/h;
	if(q<0.5) return sigma2*(6.0*(q*q*q-q*q)+1.0);
	if(q<1.0) return sigma2*(2.0*(1.0-q)*(1.0-q)*(1.0-q));
	return 0.0;
}

float dW(vec3 r, float h)
{
	float q = length(r)/h;
	if(q<0.5) return sigma2*6.0*(3.0*q*q-2.0*q)/h;
	if(q<1.0) return -sigma2*6.0*(1.0-q)*(1.0-q)/h;
	return 0.0;
}

float gradW(vec3 r, float h)
{
	return normalize(r)*dW(r,h);
}

float density(int ix)
{
	Particle pi = particles_in[ix];
	float rho = 0.0;
	for(int j=0; j<num_particles; j++)
	{
		Particle pj = particles_in[j];
		rho += mass*W(pi.pos.xyz-pj.pos.xyz, h);
	}
	return rho;
}

vec4 Fvisc(int ix)
{
	Particle pi = particles_in[ix];
	vec4 Lvi = vec4(0.0);
	for(int j=0; j<num_particles;j++)
	{
		Particle pj = particles_in[j];
		vec3 dv = pi.vel.xyz-pj.vel.xyz;
		vec3 r = pi.pos.xyz-pj.pos.xyz;
		float rho_j = rho[j];
		Lvi += mass*length(dv)/rho_j*2.0*dW(r, h)/length(r);
	}
	return mass*viscosity*Lvi;
}

vec3 Fext(int ix)
{
	return mass*g/rho[ix];
}

void update(int ix)
{
	Particle p_out = particles_in[ix];
	p_out.vel.xyz = v_star[ix]+dt/m*Fpress[ix];
	p_out.pos.xyz += dt*p_out.vel.xyz;
	particles_out[ix] = p_out;
}