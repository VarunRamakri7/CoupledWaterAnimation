#version 450

layout(local_size_x = 1024) in;

struct aabb3D
{
   vec4 mMin;
   vec4 mMax;
};

bool overlap(aabb3D a, aabb3D b)
{
   if(a.mMax.x < b.mMin.x || b.mMax.x < a.mMin.x) {return false;}
   if(a.mMax.y < b.mMin.y || b.mMax.y < a.mMin.y) {return false;}
   if(a.mMax.z < b.mMin.z || b.mMax.z < a.mMin.z) {return false;}
   return true;
}

aabb3D intersection(aabb3D a, aabb3D b)
{
   aabb3D isect;
   isect.mMin = max(a.mMin, b.mMin);
   isect.mMax = min(a.mMax, b.mMax);
   return isect;
}



//Modes
const int COMPUTE_COUNTS = 0;
const int COMPUTE_OFFSETS = 1;
const int INSERT_PARTICLES = 2;

layout(location=0) uniform int mode = COMPUTE_COUNTS;
layout(location=1) uniform int num_particles = 0;

layout(std140, binding = 10) uniform UniformGridInfo
{
	aabb3D mExtents;
	ivec4 mNumCells;
	vec4 mCellSize;
};

layout (std430, binding = 10) buffer GRID_COUNTER 
{
	int mGridCounter[];
};

layout (std430, binding = 11) buffer GRID_OFFSET 
{
	int mGridOffset[];
};

layout (std430, binding = 12) buffer INDEX_LIST 
{
	int mIndexList[];
};

struct Particle
{
   vec4 pos;
   vec4 vel; //vel.w = particle age
};

layout (std430, binding = 13) buffer PARTICLES 
{
	Particle particles[];
};

void ComputeCounts(int gid);
ivec3 ComputeCellIndex(vec3 p);
void InsertParticle(int gid);
int Index(int i, int j, int k);

void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= num_particles) return;

	if(mode==COMPUTE_COUNTS)
	{
		ComputeCounts(gid);	
	}
	if(mode==INSERT_PARTICLES)
	{
		InsertParticle(gid);
	}
}

void ComputeCounts(int gid)
{
   ivec3 cell = ComputeCellIndex(particles[gid].pos.xyz);
	int ix = Index(cell.x, cell.y, cell.z);
	atomicAdd(mGridCounter[ix], 1);   
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

void InsertParticle(int gid)
{
   ivec3 cell = ComputeCellIndex(particles[gid].pos.xyz);
	int ix = Index(cell.x, cell.y, cell.z);
   int offset = mGridOffset[ix];
   int count = atomicAdd(mGridCounter[ix], 1);
	mIndexList[offset+count] = gid;
}
