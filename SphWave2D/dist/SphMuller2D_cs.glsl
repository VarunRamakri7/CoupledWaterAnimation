#version 450

layout(local_size_x = 1024) in;

struct Particle
{
   vec4 pos;
   vec4 vel;	//.xyz=vel, .w = pressure (p)
   vec4 force; //.xyz = force, .w = density (rho)
};

float get_rho(Particle p)
{
    return p.force.w;
}

void set_rho(inout Particle p, float rho)
{
    p.force.w = rho;
}

float get_pressure(Particle p)
{
    return p.vel.w;
}

void set_pressure(inout Particle p, float press)
{
    p.vel.w = press;
}

layout (std430, binding = 0) readonly restrict buffer IN 
{
	Particle particles_in[];
};

layout (std430, binding = 1) writeonly restrict buffer OUT 
{
	Particle particles_out[];
};

//A compute shader implementation of https://lucasschuermann.com/writing/implementing-sph-in-2d 

const vec3 G = vec3(0.0, -10.0, 0.0);   // external (gravitational) forces
const float REST_DENS = 300.0;  // rest density
const float GAS_CONST = 2000.0; // const for equation of state
const float H = 8.0;           // kernel radius
const float HSQ = H * H;        // radius^2 for optimization
const float MASS = 2.5;        // assume all particles have the same mass
const float VISC = 200.0;       // viscosity constant
const float DT = 0.001;       // integration timestep
const float M_PI = 3.14159265;
const float VIEW_WIDTH = 1.5*800.0;
const float VIEW_HEIGHT = 1.5*600.0;

// smoothing kernels defined in Müller and their gradients
// adapted to 2D per "SPH Based Shallow Water Simulation" by Solenthaler et al.
const float POLY6 = 4.0 / (M_PI * pow(H, 8.0));
const float SPIKY_GRAD = -10.0 / (M_PI * pow(H, 5.0));
const float VISC_LAP = 40.0 / (M_PI * pow(H, 5.0));

float W_poly6_r2(float r2)
{
    const float k_poly6 = 4.0 / (M_PI * pow(H, 8.0));
    float d = HSQ-r2;
    return k_poly6*d*d*d;
}

float W_spiky_grad(float r)
{
    //const float k_spiky = -10.f / (M_PI * pow(H, 5.0));
    //float d = H-r;
    //return k_spiky*d*d*d;
    const float k_spiky = -30.f / (M_PI * pow(H, 5.0));
    float d = H-r;
    return k_spiky*d*d;
}

float W_visc_lap(float r)
{
    const float k_visc = 40.0 / (M_PI * pow(H, 5.0));
    return k_visc*(H-r);
}

// simulation parameters
const float EPS = H; // boundary epsilon
const float BOUND_DAMPING = -0.5;

layout(location = 0) uniform int uMode; 
layout(location = 1) uniform int num_particles;

const int MODE_INIT = 0;
const int MODE_COMPUTE_DENSITY_PRESSURE = 1;
const int MODE_COMPUTE_FORCES = 2;
const int MODE_COMPUTE_INTEGRATE = 3;

void InitParticle(int ix);
void ComputeDensityPressure(int ix);
void ComputeForces(int ix);
void Integrate(int ix);


void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= num_particles) return;

	switch(uMode)
	{
		case MODE_INIT:
			InitParticle(gid);
		break;

		case MODE_COMPUTE_DENSITY_PRESSURE:
			ComputeDensityPressure(gid);
		break;

        case MODE_COMPUTE_FORCES:
			ComputeForces(gid);
		break;

        case MODE_COMPUTE_INTEGRATE:
			Integrate(gid);
		break;
	}
}

vec4 init_grid(int ix);

void InitParticle(int ix)
{
	//particles_out[ix].pos = vec4(0.0, 0.0, 0.0, 1.0);
	//particles_out[ix].pos = init_grid(ix);

    //adjust for view width/height
    vec4 p = init_grid(ix);
    p.xy += vec2(1.0);
    p.xy *= 0.5*vec2(VIEW_WIDTH, VIEW_HEIGHT);
    p.x += VIEW_WIDTH/4.0;
    particles_out[ix].pos = p;
	particles_out[ix].vel = vec4(0.0);
    particles_out[ix].force = vec4(0.0);
}

vec4 init_grid(int ix)
{
	int cols = 64;
	ivec2 ij = ivec2(ix%cols, ix/cols);
	vec2 p = 0.01*(ij-ivec2(cols/2));
	return vec4(p, 0.0, 1.0);
}

void ComputeDensityPressure(int ix)
{
	Particle pi = particles_in[ix];
	
	float rho = 0.0;
    for (int j=0; j<num_particles; j++)
    {
		Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        
		float r2 = dot(rij,rij);
        if (r2 < HSQ)
        {
            // this computation is symmetric
			//rho += MASS * POLY6 * pow(HSQ - r2, 3.0);
            rho += MASS * W_poly6_r2(r2);
        }
    }
    set_rho(pi, rho);
    //set_pressure(pi, GAS_CONST * (rho - REST_DENS));
    set_pressure(pi, GAS_CONST*REST_DENS * (rho / REST_DENS-1.0));

	particles_out[ix] = pi;
}

void ComputeForces(int ix)
{
	Particle pi = particles_in[ix];
    
    vec2 fpress = vec2(0.0);
    vec2 fvisc = vec2(0.0);
    for (int j=0; j<num_particles; j++)
	{
		Particle pj = particles_in[j];
        if (ix == j)
        {
            continue;
        }

        vec2 rij = pj.pos.xy - pi.pos.xy;
        float r = length(rij);
		vec2 uij = rij/r;

        if (r < H)
        {
            // compute pressure force contribution
            //fpress += -uij * MASS * (get_pressure(pi) + get_pressure(pj)) / (2.0 * get_rho(pj)) * SPIKY_GRAD * pow(H - r, 3.0);
            fpress += -uij * MASS * (get_pressure(pi) + get_pressure(pj)) / (2.0 * get_rho(pj)) * W_spiky_grad(r);
            // compute viscosity force contribution
            //fvisc += VISC * MASS * (pj.vel.xy - pi.vel.xy) / get_rho(pj) * VISC_LAP * (H - r);
            fvisc += VISC * MASS * (pj.vel.xy - pi.vel.xy) / get_rho(pj) * W_visc_lap(r);
        }
    }
    vec2 fgrav = G.xy * MASS / get_rho(pi);
    pi.force.xy = fpress + fvisc + fgrav;
    
    //INTEGRATE
    // forward Euler integration
    pi.vel.xy += DT * pi.force.xy / get_rho(pi);
    pi.pos.xy += DT * pi.vel.xy;
    
    // enforce boundary conditions
    if (pi.pos[0] - EPS < 0.f)
    {
        pi.vel[0] *= BOUND_DAMPING;
        pi.pos[0] = EPS;
    }
    if (pi.pos[0] + EPS > VIEW_WIDTH)
    {
        pi.vel[0] *= BOUND_DAMPING;
        pi.pos[0] = VIEW_WIDTH - EPS;
    }
    if (pi.pos[1] - EPS < 0.f)
    {
        pi.vel[1] *= BOUND_DAMPING;
        pi.pos[1] = EPS;
    }
    if (pi.pos[1] + EPS > VIEW_HEIGHT)
    {
        pi.vel[1] *= BOUND_DAMPING;
        pi.pos[1] = VIEW_HEIGHT - EPS;
    }
    
    particles_out[ix] = pi;

    //INTEGRATE
	particles_out[ix] = pi;
}

void Integrate(int ix)
{
    return;
    Particle pi = particles_in[ix];
    
    // forward Euler integration
    pi.vel.xy += DT * pi.force.xy / get_rho(pi);
    pi.pos.xy += DT * pi.vel.xy;
    
    // enforce boundary conditions
    if (pi.pos[0] - EPS < 0.f)
    {
        pi.vel[0] *= BOUND_DAMPING;
        pi.pos[0] = EPS;
    }
    if (pi.pos[0] + EPS > VIEW_WIDTH)
    {
        pi.vel[0] *= BOUND_DAMPING;
        pi.pos[0] = VIEW_WIDTH - EPS;
    }
    if (pi.pos[1] - EPS < 0.f)
    {
        pi.vel[1] *= BOUND_DAMPING;
        pi.pos[1] = EPS;
    }
    if (pi.pos[1] + EPS > VIEW_HEIGHT)
    {
        pi.vel[1] *= BOUND_DAMPING;
        pi.pos[1] = VIEW_HEIGHT - EPS;
    }
    
    particles_out[ix] = pi;
}