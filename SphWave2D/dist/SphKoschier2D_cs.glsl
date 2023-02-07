#version 450
#pragma optimize (off)

layout(local_size_x = 1024) in;

struct Particle
{
   vec4 pos;
   vec4 vel;    //.xyz = vel,   .w = pressure (p)
   vec4 acc;  //.xyz = force, .w = density (rho)
};

float get_rho(Particle p)
{
    return p.acc.w;
}

void set_rho(inout Particle p, float rho)
{
    p.acc.w = rho;
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

layout (std430, binding = 9) restrict buffer NONE 
{
	vec4 junk;
};

//A compute shader implementation of https://lucasschuermann.com/writing/implementing-sph-in-2d 

const vec3 G = vec3(0.0, -10.0, 0.0);   // external (gravitational) forces m/s^2
uniform float REST_DENS = 1700.0;       // rest density (kg/m^3)
uniform vec2 REST_DENS_range = vec2(1000.0, 3000.0);
uniform float GAS_CONST = 2000.0;       // const for equation of state
uniform vec2 GAS_CONST_range = vec2(1000.0, 3000.0);
uniform float H = 0.02;                 // kernel radius (m)
uniform vec2 H_range = vec2(0.0001, 0.1000);
const float HSQ = H * H;                // radius^2 for optimization (m^2)
uniform float MASS = 0.20;              // assume all particles have the same mass (kg)
uniform vec2 MASS_range = vec2(0.001, 1.0);
uniform float VISC = 0.001;             // viscosity constant (Pa*s)
uniform vec2 VISC_range = vec2(0.0, 0.01);
uniform float DT = 0.0012;              // integration timestep (s)
uniform vec2 DT_range = vec2(0.0, 0.01);

const float M_PI = 3.14159265;
const float VIEW_WIDTH = 1.0;
const float VIEW_HEIGHT = 1.0;

const float k2 = 40.0/(7.0*M_PI*HSQ);

void touch_range()
{
    junk.xy = REST_DENS_range;
    junk.xy += GAS_CONST_range;
    junk.xy += H_range;
    junk.xy += MASS_range;
    junk.xy += VISC_range;
    junk.xy += DT_range;
 
}

float W_cubic(float r)
{
    float q = r/H;
    if(q>=1.0) return 0.0;
    if(q<=0.5)
    {
        return k2*(6.0*q*q*(q-1.0)+1.0);
    }
    else
    {
        float q1 = 1.0-q;
        return k2*2.0*q1*q1*q1;
    }
}

float W_cubic_grad(float r)
{
    float q = r/H;
    if(q>=1.0) return 0.0;
    if(q<=0.5)
    {
        return 6.0*k2*q*(3.0*q-2.0)/H;
    }
    else
    {
        float q1 = 1.0-q;
        return -6.0*k2*(q1*q1)/H;
    }
}

float W_cubic_lap(float r)
{
    float q = r/H;
    if(q<=0.5)
    {
        return k2*6.0*(6.0*q-2.0)/HSQ;
    }
    else
    {
        return k2*12.0*(1.0-q)/HSQ;
    }
}

// simulation parameters
const float EPS = 0.25*H; // boundary epsilon
const float BOUND_DAMPING = -0.5;

void boundary(inout Particle pi)
{
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
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

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

void Splitting1(int ix);
void Splitting2(int ix);
void Splitting3(int ix);

float CompressibleStateEqn(float rho)
{
    return GAS_CONST * (rho/REST_DENS - 1.0);
}

float WeaklyCompressibleStateEqn(float rho)
{
    float gamma = 7.0;
    return GAS_CONST/gamma * (pow(rho/REST_DENS, gamma) - 1.0);
}

void main()
{
    if(uMode==-1) touch_range();

	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= num_particles) return;

	switch(uMode)
	{
		case MODE_INIT:
			InitParticle(gid);
		break;

		case MODE_COMPUTE_DENSITY_PRESSURE:
			ComputeDensityPressure(gid);
            //Splitting1(gid);
		break;

        case MODE_COMPUTE_FORCES:
			ComputeForces(gid);
            //Splitting1(gid);
		break;

        case MODE_COMPUTE_INTEGRATE:
			//Integrate(gid);
            //Splitting2(gid);
		break;
        case MODE_COMPUTE_INTEGRATE+1:
			//Integrate(gid);
            //Splitting3(gid);
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
    p.x -= VIEW_WIDTH/4.0;
    p.xy += 0.001*vec2(rand(p.xy), rand(p.yx)); //jitter
    particles_out[ix].pos = p;
	particles_out[ix].vel = vec4(0.0);
    particles_out[ix].acc = vec4(0.0, 0.0, 0.0, REST_DENS);
}

vec4 init_grid(int ix)
{
	int cols = 32;
	ivec2 ij = ivec2(ix%cols, ix/cols);
	vec2 p = 0.02*(ij-ivec2(cols/2));
	return vec4(p, 0.0, 1.0);
}

void ComputeDensityPressure(int ix)
{
	Particle pi = particles_in[ix];
    //verlet 1/2
    pi.vel.xy += 0.5*DT * pi.acc.xy; //v(t+0.5DT)
    pi.pos.xy += DT * pi.vel.xy;     //x(t+DT)
	
	float rho = 0.0;
    const float c_rho = MASS;
    for (int j=0; j<num_particles; j++)
    {
		Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        
		float r2 = dot(rij,rij);
        if (r2 < HSQ)
        {
            rho += W_cubic(sqrt(r2));
        }
    }
    rho = c_rho * rho;
    rho = max(REST_DENS, rho); // clamp density
    set_rho(pi, rho);

    float press_i = CompressibleStateEqn(rho);
    //float press_i = WeaklyCompressibleStateEqn(rho);
    set_pressure(pi, press_i);

	particles_out[ix] = pi;
}

void ComputeForces(int ix)
{
	Particle pi = particles_in[ix];
    float rho_i = get_rho(pi);
    vec2 acc_press = vec2(0.0);
    vec2 acc_visc = vec2(0.0);
    
    vec2 acc_grav = G.xy;
    float c_visc = -VISC*8.0*MASS;
    //float c_visc = -VISC*MASS;
    float c_press = MASS;

    float acc_press_i = get_pressure(pi)/(rho_i*rho_i);
    for (int j=0; j<num_particles; j++)
	{
        if (ix == j)
        {
            continue;
        }
        Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        float r = length(rij);

        if (r < H)
        {
            float W = W_cubic_grad(r);
            vec2 uij = rij/r;
            float rho_j = get_rho(pj);
            // compute pressure force contribution
            acc_press += (acc_press_i + get_pressure(pj)/(rho_j*rho_j)) * W*uij;
            // compute viscosity force contribution
            vec2 vij = pj.vel.xy - pi.vel.xy;
            acc_visc += 1.0/rho_j * dot(vij, rij)/(r*r + 0.01*HSQ) * W*uij;
        }
    }
    acc_visc *= c_visc;
    acc_press *= c_press;
    
    
    //INTEGRATE
    // forward Euler integration
    //pi.acc.xy = acc_press + acc_visc + acc_grav;
    //pi.vel.xy += DT * pi.acc.xy;
    //pi.pos.xy += DT * pi.vel.xy;

    
    //verlet 2/2
    pi.acc.xy = acc_press + acc_visc + acc_grav;
    pi.vel.xy += 0.5*DT * pi.acc.xy;

    // enforce boundary conditions
    boundary(pi);
    
    particles_out[ix] = pi;

}

void Splitting1(int ix)
{
    Particle pi = particles_in[ix];
    float rho_i = get_rho(pi);
    vec2 fvisc = vec2(0.0);
    vec2 fsep = vec2(0.0);
    vec2 fgrav = G.xy;
    for (int j=0; j<num_particles; j++)
    {
        if (ix == j)
        {
        continue;
        }
        Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        float r = length(rij);
		
        if (r < H)
        {
            vec2 uij = rij/r;
            float rho_j = get_rho(pj);
            // compute viscosity force contribution
            vec2 vij = pj.vel.xy - pi.vel.xy;
            fvisc += -VISC*8.0*MASS/rho_j * dot(vij, rij)/(r*r + 0.01*HSQ) * W_cubic_grad(r)*uij;
            fsep += -250.0*uij*(1.0-smoothstep(0.2*H, 0.3*H, r)); //HACK to keep particles from collapsing
        }
    }
    pi.acc.xy = fvisc + fgrav + fsep;
    
    //INTEGRATE
    // forward Euler integration
    pi.vel.xy += DT * pi.acc.xy; //This is now v*

    particles_out[ix] = pi;
}

void Splitting2(int ix)
{
    Particle pi = particles_in[ix];
    float rho_star = get_rho(pi);
    for (int j=0; j<num_particles; j++)
    {
        if (ix == j)
        {
        continue;
        }
        Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        float r = length(rij);
        
        if (r < H)
        {
            vec2 uij = rij/r;
            vec2 dv_star = pj.vel.xy-pi.vel.xy;
            rho_star += DT*MASS*dot(dv_star,uij)*W_cubic_grad(r);
            //rho_star += DT*MASS*length(dv_star)*W_cubic_grad(r);
        }
    }
    //set_rho(pi, rho_star);
    float press_i = CompressibleStateEqn(rho_star);
    //float press_i = WeaklyCompressibleStateEqn(rho);
    set_pressure(pi, press_i);
    
    particles_out[ix] = pi;
}

void Splitting3(int ix)
{
    Particle pi = particles_in[ix];
    float rho_i = get_rho(pi);
    vec2 fpress = vec2(0.0);
    for (int j=0; j<num_particles; j++)
    {
        if (ix == j)
        {
        continue;
        }
        Particle pj = particles_in[j];
        vec2 rij = pj.pos.xy - pi.pos.xy;
        float r = length(rij);
        
        if (r < H)
        {
            vec2 uij = rij/r;
            float rho_j = get_rho(pj);
            // compute pressure force contribution
            fpress += MASS * (get_pressure(pi)/(rho_i*rho_i) + get_pressure(pj)/(rho_j*rho_j)) * W_cubic_grad(r)*uij;
        }
    }
    pi.acc.xy += fpress;
    //INTEGRATE
    // forward Euler integration
    pi.vel.xy += DT * fpress;
    pi.pos.xy += DT * pi.vel.xy;

    // enforce boundary conditions
    boundary(pi);
    
    particles_out[ix] = pi;
}

void Integrate(int ix)
{
    return;
    Particle pi = particles_in[ix];
    
    // forward Euler integration
    pi.vel.xy += DT * pi.acc.xy;
    pi.pos.xy += DT * pi.vel.xy;
    
    // enforce boundary conditions
    boundary(pi);
    
    particles_out[ix] = pi;
}