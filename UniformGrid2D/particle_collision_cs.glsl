#version 450
#extension GL_NV_shader_atomic_float : enable

layout(local_size_x = 1024) in;


struct aabb3D
{
   vec4 mMin;
   vec4 mMax;
};

layout(std140, binding = 0) uniform UniformGridInfo
{
	aabb3D mExtents;
	ivec4 mNumCells;
	vec4 mCellSize;
};

layout(std140, binding = 1) uniform Params
{
	vec4 g; //gravity
   float dt;
   float damping;
   float k; //SOR parameter
   float k_static;
   float sleep_thresh;
   float mu_k; //kinetic friction
   float mu_s; //static friction
   float e; //coefficient of restitution
   int n; //num objects
   int substeps; //number of collision substeps
} mParams;

//Modes
const int COMPUTE_COUNTS = 0;
const int COMPUTE_OFFSETS = 1;
const int INSERT_BOXES = 2;
const int PREANIMATE = 3;
const int COLLISION_QUERY = 4;
const int POSTANIMATE = 5;
          
layout(location = 0) uniform int num_particles;
layout(location = 1) uniform float time;
layout(location = 3) uniform int mode = COMPUTE_COUNTS;

const float PI = 3.1415926535;

struct Particle
{
   vec4 pos;
   vec4 vel; //v.w = particle age
};

struct PbdObj
{
   vec4 color;
   vec4 x;
   vec4 v;
   vec4 p;
   vec4 dp_t;
   vec4 dp_n;
   vec4 hw;
   float rm;
   float n;
   float e;
   float padding;
};

//SSBO holds array of particle structs.
//Be sure to check the std430 rules for alignment
layout (std430, binding = 0) buffer GRID_COUNTER 
{
	int mGridCounter[];
};

layout (std430, binding = 1) buffer GRID_OFFSET 
{
	int mGridOffset[];
};

layout (std430, binding = 2) buffer INDEX_LIST 
{
	int mIndexList[];
};

layout (std430, binding = 3) buffer OBJS 
{
	PbdObj mObj[];
};

layout (std430, binding = 4) buffer PARTICLES 
{
	Particle particles[];
};

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

ivec3 ComputeCellIndex(vec3 p);
int Index(int i, int j, int k);
void CollisionQuery(int gid);
void PreAnimate(int gid);
void PostAnimate(int gid);
vec2 boxIntersection( in vec3 ro, in vec3 rd, in vec3 rad, out vec3 oN );

void main(void)
{
   const int ix = int(gl_GlobalInvocationID.x);

   if(mode==PREANIMATE)
	{
		PreAnimate(ix);
	}
	if(mode==COLLISION_QUERY)
	{
		CollisionQuery(ix);
	}
   if(mode==POSTANIMATE)
	{
		PostAnimate(ix);
	}
}

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 v0(vec3 p)
{
	vec3 v = vec3(sin(-p.y*10.0+time/2.0-10.0), sin(-p.x*10.0+1.2*time+10.0), cos(+7.0*p.z -5.0*p.x + time+10.0));
   return v;
}

ivec3 ComputeCellIndex(vec3 p)
{
   p = p-mExtents.mMin.xyz;
   ivec3 cell = ivec3(floor(p/mCellSize.xyz));
   cell = clamp(cell, ivec3(0), mNumCells.xyz-ivec3(1));
   return cell;
}

int Index(int i, int j, int k)
{
	return (i*mNumCells.y + j)*mNumCells.x + k;
}

void PreAnimate(int i)
{
   //Apply external forces, damping
   vec4 v = particles[i].vel;
   vec4 p = particles[i].pos;
   float age = v.w-1.0;

   if(age <= 0.0)
   {
      vec3 seed = vec3(0.0001*float(i), time/100.0, 0.0); //seed for the random number generator
      age = 200.0 + 150.0*rand(seed.xy);
      //Pseudorandom position
      p.xyz = 0.01*(rand3(p.xyz+seed))+vec3(0.0, 1.0, -0.65); 
      v.xyz = 0.15*(rand3(p.xyz)-vec3(0.5))+vec3(0.0, -0.3, 0.0);
   }

   
   v.xyz = mParams.damping*(v.xyz + mParams.dt*mParams.g.xyz);
   v.xyz += 0.05*v0(p.xyz)*mParams.dt;

   //Compute estimates
   p += mParams.dt*v;

   particles[i].pos = vec4(p.xyz, 1.0);
   particles[i].vel = vec4(v.xyz, age);
}

void PostAnimate(int i)
{
   vec4 v = particles[i].vel;
   //sleeping
   if(length(v.xyz) < mParams.sleep_thresh)
   {
      //v.xyz = vec3(0.0);
   }
   particles[i].vel.xyz = v.xyz;
}

void CollisionQuery(int gid)
{
   vec4 p = particles[gid].pos;
   vec4 v = particles[gid].vel;
   vec4 x = p-mParams.dt*v;

	aabb3D query_aabb = aabb3D(p, p);
   
   //Collide with extents
   for(int c = 0; c<3; c++) //dimension: x, y, or z
   {
      float overlap[2] = {2.0*mExtents.mMin[c]-query_aabb.mMin[c], query_aabb.mMax[c] - 2.0*mExtents.mMax[c]};
      float dir[2] = {+1.0f, -1.0f};
      for(int m=0; m<2; m++) //min or max
      {
         if(overlap[m] > 0.0f)
         {
            p[c] += dir[m]*overlap[m];
            v[c] = -0.75*v[c];
         }
      }
   }

	//These are the cells the query overlaps
   ivec3 cell = ComputeCellIndex(p.xyz);
   int ix = Index(cell.x,cell.y,cell.z);
   
//*
   int offset = mGridOffset[ix];
   int count = mGridCounter[ix];
   for(int list_index = offset; list_index<offset+count; list_index++)
   {
      int box_index = mIndexList[list_index];
     
      //transform to box space
      vec3 box_cen = mObj[box_index].x.xyz;
      vec3 ro = x.xyz-box_cen;
      float t_max = distance(p.xyz, x.xyz)+1e-8;
      vec3 rd = (p.xyz-x.xyz)/t_max;
      vec3 oN;
      vec3 rad = mObj[box_index].hw.xyz;
      vec2 t = boxIntersection(ro, rd, rad, oN); 
      if(t.x > 0.0 && t.x < t_max)
      {
         p.xyz = x.xyz+t.x*rd;
         vec3 vN = dot(v.xyz, oN)*oN;
         vec3 vT = v.xyz-vN;
         v.xyz = vT-0.75*vN;
      }
   }
//*/
   particles[gid].pos = vec4(p.xyz, 1.0);
   particles[gid].vel.xyz = v.xyz;
}

vec2 boxIntersection( in vec3 ro, in vec3 rd, in vec3 rad, out vec3 oN ) 
{
    vec3 m = 1.0/rd;
    vec3 n = m*ro;
    vec3 k = abs(m)*rad;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
	
    if( tN>tF || tF<0.0) return vec2(-1.0); // no intersection
    
    oN = -sign(rd)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz);

    return vec2( tN, tF );
}