#version 450
#pragma optimize (off)

layout(local_size_x = 1024) in;

struct aabb2D
{
   vec2 mMin;
   vec2 mMax;
};

bool overlap(aabb2D a, aabb2D b)
{
   if(a.mMax.x < b.mMin.x || b.mMax.x < a.mMin.x) {return false;}
   if(a.mMax.y < b.mMin.y || b.mMax.y < a.mMin.y) {return false;}
   return true;
}

aabb2D intersection(aabb2D a, aabb2D b)
{
   aabb2D isect;
   isect.mMin = max(a.mMin, b.mMin);
   isect.mMax = min(a.mMax, b.mMax);
   return isect;
}

vec2 center(aabb2D a)
{
   return 0.5*(a.mMin + a.mMax);
}

float area(aabb2D box)
{
   vec2 diff = box.mMax-box.mMin;
   return diff.x*diff.y;
}


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

layout(std140, binding = 0) uniform UniformGridInfo
{
	aabb2D mExtents;
	ivec2 mNumCells;
	vec2 mCellSize;
};

ivec2 ComputeCellIndex(vec2 p)
{
   p = p-mExtents.mMin;
   ivec2 cell = ivec2(floor(p/mCellSize));
   cell = clamp(cell, ivec2(0), mNumCells-ivec2(1));
   return cell;
}

int Index(int i, int j)
{
	return i*mNumCells.y + j;
}

layout (std430, binding = 0) readonly restrict buffer IN 
{
	Particle particles_in[];
};

layout (std430, binding = 1) writeonly restrict buffer OUT 
{
	Particle particles_out[];
};

layout (std430, binding = 2) buffer GRID_COUNTER 
{
	int mGridCounter[];
};

layout (std430, binding = 3) buffer GRID_OFFSET 
{
	int mGridOffset[];
};

layout (std430, binding = 4) buffer INDEX_LIST 
{
	int mIndexList[];
};

layout (std430, binding = 9) restrict buffer NONE 
{
	vec4 junk;
};

const vec3 G = vec3(0.0, -9.8, 0.0);   // external (gravitational) forces m/s^2
//TODO fix uniform grid for points, not boxes
const float PARTICLE_RADIUS = 0.025;
const float PARTICLE_DIAM = 2.0*PARTICLE_RADIUS;
const float H = 4.0*PARTICLE_RADIUS;//SUPPORT_RADIUS
const float HSQ = H * H;
const float REST_DENS = 1000.0;
const float VISC = 0.05;
const float MASS = PARTICLE_DIAM*PARTICLE_DIAM*REST_DENS;
const float DT = 0.002;
const float GAS_CONST = 35000.0;


const float M_PI = 3.14159265;
const int WIDTH = 32; //width and height of initial particle grid
const int HEIGHT = 32;
const float VIEW_WIDTH = 2.0*4.8;//3*WIDTH*PARTICLE_DIAM;
const float VIEW_HEIGHT = 2.0*4.8;//3*HEIGHT*PARTICLE_DIAM;

const float k2 = 40.0/(7.0*M_PI*HSQ);
uniform float PSI = REST_DENS/(1.5*k2);
uniform float time = 0.0;

/*
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
uniform float PSI = 0.5; //boundary particle mass factor

float particleRadius() {return H/4.0;}
float particleDiameter(){return H/2.0;} //particleRadius*2.0

const float M_PI = 3.14159265;
const float VIEW_WIDTH = 90.0;
const float VIEW_HEIGHT = 90.0;
*/


void touch_range()
{
/*
    junk.xy = REST_DENS_range;
    junk.xy += GAS_CONST_range;
    junk.xy += H_range;
    junk.xy += MASS_range;
    junk.xy += VISC_range;
    junk.xy += DT_range;
 */
}

float sdPlane( vec2 p, vec3 plane)
{
    return dot(plane.xy, p)+plane.z;
}

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}


//vec4: nx, ny, sd, id
vec4 opU( vec4 d1, vec4 d2 )
{
	return (d1.z<d2.z) ? d1 : d2;
}

float Vb(float d)
{
    //return M_PI*H*H*smoothstep(H, -H, d);

    const float R = H;
    float h = R-d;
    if(d>R) return 0.0;
    if(d<-R) return M_PI*R*R;
    return R*R*acos((R-h)/R)- (R-h)*sqrt(2.0*R*h-h*h);
}

vec4 boundary_sdf(vec2 p)
{
    //disable
    //return vec4(1e6);

    const vec3 plane0 = vec3(0.0, 1.0, -PARTICLE_RADIUS);
    const vec3 plane1 = vec3(1.0, 0.0, -PARTICLE_RADIUS);
    const vec3 plane2 = vec3(-1.0, 0.0, VIEW_WIDTH-PARTICLE_RADIUS);
    const vec2 cen = vec2(0.5*VIEW_WIDTH+3.0*cos(time), 0.5*VIEW_HEIGHT+3.0*sin(time));
    const float r = 1.0;

    float d0 = sdPlane(p, plane0);
    float d1 = sdPlane(p, plane1);
    float d2 = sdPlane(p, plane2);
    float d3 = sdCircle(p-cen, r);

    vec4 res = vec4(plane0.xy, d0, 0.0);
    res = opU(res, vec4(plane1.xy, d1, 1.0));
    res = opU(res, vec4(plane2.xy, d2, 2.0));
    res = opU(res, vec4(normalize(p-cen), d3, 3.0));
    return res;
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
    if(q>=1.0) return 0.0;
    if(q<=0.5)
    {
        return k2*6.0*(6.0*q-2.0)/HSQ;
    }
    else
    {
        return k2*12.0*(1.0-q)/HSQ;
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

float CompressibleStateEqn(float rho)
{
    return GAS_CONST * (rho/REST_DENS - 1.0);
}

float WeaklyCompressibleStateEqn(float rho)
{
    float gamma = 3.0;
    return GAS_CONST * (pow(rho/REST_DENS, gamma) - 1.0);
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
            
		break;

        case MODE_COMPUTE_FORCES:
			ComputeForces(gid);
            
		break;

        case MODE_COMPUTE_INTEGRATE:
			
		break;
        case MODE_COMPUTE_INTEGRATE+1:
			
		break;
	}
}

vec4 init_grid(int ix);

void InitParticle(int ix)
{
    //adjust for view width/height
    vec4 p = init_grid(ix);
   
    //p.xy *= 1.0*vec2(VIEW_WIDTH, VIEW_HEIGHT);
    p.xy += 0.1/6.0*vec2(VIEW_WIDTH, VIEW_HEIGHT);
    
    //p.xy += 0.001*vec2(rand(p.xy), rand(p.yx)); //jitter
    particles_out[ix].pos = p;
	particles_out[ix].vel = vec4(0.0);
    particles_out[ix].acc = vec4(0.0, 0.0, 0.0, REST_DENS);
}

vec4 init_grid(int ix)
{
	int cols = WIDTH;
    int rows = num_particles/cols;
	ivec2 ij = ivec2(ix%cols, ix/cols);
	vec2 p = PARTICLE_DIAM*vec2(ij);
	return vec4(p, 0.0, 1.0);
}

void ComputeDensityPressure(int ix)
{
	Particle pi = particles_in[ix];
    //verlet 1/2
    //pi.vel.xy += 0.5*DT * pi.acc.xy; //v(t+0.5DT)
    //pi.pos.xy += DT * pi.vel.xy;     //x(t+DT)

    vec4 db = boundary_sdf(pi.pos.xy);
    //*
    if(db.z < 0.0)
    {
        pi.pos.xy -= 0.75*db.z*db.xy; //unpenetrate (db.z is negative)
        pi.vel.xy -= 1.75*min(0.0, dot(pi.vel.xy, db.xy))*db.xy;
    }
	//*/
    //*
	if(pi.pos.y > VIEW_HEIGHT)
	{
		particles_out[ix] = pi;
		return;
	}
	//*/
	float rho = 0.0;
    const float c_rho = MASS;

    aabb2D query_aabb = aabb2D(pi.pos.xy-vec2(H), pi.pos.xy+vec2(H));
    //These are the cells the query overlaps
    ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
    ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

    for(int i=cell_min.x; i<=cell_max.x; i++)
    {
	    for(int j=cell_min.y; j<=cell_max.y; j++)
	    {
		    int cell_ix = Index(i,j);
         
		    int start = mGridOffset[cell_ix]; //TODO: rename to mCellStart
		    int count = mGridCounter[cell_ix];//TODO: rename to mCellCount
		    for(int list_index = start; list_index<start+count; list_index++)
		    {
			    int jx = mIndexList[list_index];//TODO: rename mContentList
                Particle pj = particles_in[jx];
                vec2 rij = pi.pos.xy - pj.pos.xy;
        
                float r2 = dot(rij,rij);
                if (r2 < HSQ)
                {
                    rho += W_cubic(sqrt(r2));
                }
		    }       
	    }
    }
    rho = c_rho * rho;

    //add boundary component
    if(db.z<H)
    {
        rho += PSI*W_cubic(max(0.0, db.z+0.0*PARTICLE_RADIUS));
    }

    rho = max(REST_DENS, rho); // clamp density
    set_rho(pi, rho);

    //float press_i = CompressibleStateEqn(rho);
    float press_i = WeaklyCompressibleStateEqn(rho);
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
    //float c_visc = VISC*MASS;
    float c_press = MASS;

    float acc_press_i = get_pressure(pi)/(rho_i*rho_i);

    aabb2D query_aabb = aabb2D(pi.pos.xy-vec2(H), pi.pos.xy+vec2(H));
    //These are the cells the query overlaps
    ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
    ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

    for(int i=cell_min.x; i<=cell_max.x; i++)
    {
	    for(int j=cell_min.y; j<=cell_max.y; j++)
	    {
		    int cell_ix = Index(i,j);
		    int start = mGridOffset[cell_ix];
		    int count = mGridCounter[cell_ix];
		    for(int list_index = start; list_index<start+count; list_index++)
		    {
			    int jx = mIndexList[list_index];
                Particle pj = particles_in[jx];
			  
			    if(jx != ix) //don't process self (ix)
			    {
                    vec2 rij = pi.pos.xy - pj.pos.xy;
                    float r = length(rij);

                    if (r < H)
                    {
                        float Wgrad = W_cubic_grad(r);
                        vec2 uij = rij/r;
                        float rho_j = get_rho(pj);
                        // compute pressure force contribution
                        acc_press -= (acc_press_i + get_pressure(pj)/(rho_j*rho_j)) * Wgrad*uij;
                        // compute viscosity force contribution
                        vec2 vij = pi.vel.xy - pj.vel.xy;
                        acc_visc -= 1.0/rho_j * dot(vij, rij)/(r*r + 0.01*HSQ) * Wgrad*uij;
                        //float Wij = W_cubic(r);
                        //acc_visc += 1.0/(rho_j*DT)*Wij*vij;
                    }    
			    }
		    }       
	    }
    }

    acc_visc *= c_visc;
    acc_press *= c_press;
    
    vec4 db = boundary_sdf(pi.pos.xy);
    if(db.z<H)
    {
        float Wgrad = W_cubic_grad(max(0.0, db.z+0.0*PARTICLE_RADIUS));
        vec2 uij = -db.xy;
        //add boundary component to pressure force
        acc_press += PSI*acc_press_i*Wgrad*uij;
    }

    //INTEGRATE
    //forward Euler integration
    pi.acc.xy = acc_press + acc_visc + acc_grav;
    pi.vel.xy += DT * pi.acc.xy;
    pi.pos.xy += DT * pi.vel.xy;

    //verlet 2/2
    //pi.acc.xy = acc_press + acc_visc + acc_grav;
    //pi.vel.xy += 0.5*DT * pi.acc.xy;

    particles_out[ix] = pi;

}