#version 450


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

bool point_in_aabb(vec2 pt, aabb2D box)
{
	if(all(greaterThan(pt, box.mMin)) && all(lessThan(pt, box.mMax))) return true;
	return false;
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



//Modes
const int COMPUTE_COUNTS = 0;
const int COMPUTE_OFFSETS = 1;
const int INSERT_BOXES = 2;

layout(location=0) uniform int mode = COMPUTE_COUNTS;
layout(location=1) uniform int num_boxes = 0;
layout(location=2) uniform vec2 box_hw = vec2(0.02);

layout(std140, binding = 0) uniform UniformGridInfo
{
	aabb2D mExtents;
	ivec2 mNumCells;
	vec2 mCellSize;
};

layout (std430, binding = 0) readonly restrict buffer IN 
{
	Particle mParticles[];
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


void ComputeCounts(int gid);
ivec2 ComputeCellIndex(vec2 p);
void InsertPoint(int gid);
int Index(int i, int j);
void CollisionQuery(int gid);
void Collide(int i, int j, aabb2D box_i, aabb2D box_j);

void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= num_boxes) return;

	if(mode==COMPUTE_COUNTS)
	{
		ComputeCounts(gid);	
	}
	if(mode==INSERT_BOXES)
	{
		InsertPoint(gid);
	}
}

void ComputeCounts(int gid)
{
	
	if(point_in_aabb(mParticles[gid].pos.xy, mExtents)==false)
	{
		return;
	}

	//SPH particle is a point and olny belongs to a single cell
	ivec2 cell = ComputeCellIndex(mParticles[gid].pos.xy);
	int ix = Index(cell.x, cell.y);
	atomicAdd(mGridCounter[ix], 1);
	return;
	
	/*
	ivec2 cell_min = ComputeCellIndex(mParticles[gid].pos.xy-box_hw);
	ivec2 cell_max = ComputeCellIndex(mParticles[gid].pos.xy+box_hw);

	for(int i=cell_min.x; i<=cell_max.x; i++)
	{
		for(int j=cell_min.y; j<=cell_max.y; j++)
		{
			int ix = Index(i,j);
			atomicAdd(mGridCounter[ix], 1);
		}
	}
	*/
}

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

void InsertPoint(int gid)
{
	if(point_in_aabb(mParticles[gid].pos.xy, mExtents)==false)
	{
		return;
	}
	
	ivec2 cell = ComputeCellIndex(mParticles[gid].pos.xy);
	int ix = Index(cell.x, cell.y);
    int offset = mGridOffset[ix];
    int count = atomicAdd(mGridCounter[ix], 1);
    mIndexList[offset+count] = gid;

	/*
    ivec2 cell_min = ComputeCellIndex(mParticles[gid].pos.xy-box_hw);
    ivec2 cell_max = ComputeCellIndex(mParticles[gid].pos.xy+box_hw);
   
    //insert box into each cell it overlaps
    for(int i=cell_min.x; i<=cell_max.x; i++)
    {
        for(int j=cell_min.y; j<=cell_max.y; j++)
        {
            int ix = Index(i,j);
            int offset = mGridOffset[ix];
            int count = atomicAdd(mGridCounter[ix], 1);
            mIndexList[offset+count] = gid;
        }
    }
	*/
}

void CollisionQuery(int gid)
{
	aabb2D query_aabb = aabb2D(mParticles[gid].pos.xy-box_hw, mParticles[gid].pos.xy+box_hw);
	//These are the cells the query overlaps
	ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
	ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

	for(int i=cell_min.x; i<=cell_max.x; i++)
	{
		for(int j=cell_min.y; j<=cell_max.y; j++)
		{
			int ix = Index(i,j);
         
			int offset = mGridOffset[ix];
			int count = mGridCounter[ix];
			for(int list_index = offset; list_index<offset+count; list_index++)
			{
				int box_index = mIndexList[list_index];
				aabb2D box = aabb2D(mParticles[box_index].pos.xy-box_hw, mParticles[box_index].pos.xy+box_hw);
				ivec2 home = max(cell_min, ComputeCellIndex(box.mMin));
				if(home == ivec2(i,j) && overlap(query_aabb, box))
				{
					//Collide(gid, box_index, query_aabb, box);
				}
			}       
		}
	}
}
