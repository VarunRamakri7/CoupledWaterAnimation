#version 450
#extension GL_NV_shader_atomic_float : enable

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

//Modes
const int COMPUTE_COUNTS = 0;
const int COMPUTE_OFFSETS = 1;
const int INSERT_BOXES = 2;
const int COLLISION_QUERY = 3;
const int ANIMATE = 4;

layout(location=0) uniform int mode = COMPUTE_COUNTS;
layout(location=1) uniform int num_boxes = 0;

layout(std140, binding = 0) uniform UniformGridInfo
{
	aabb2D mExtents;
	ivec2 mNumCells;
	vec2 mCellSize;
};

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

layout (std430, binding = 3) buffer BOXES 
{
	aabb2D mBoxes[];
};

layout (std430, binding = 4) buffer VELOCITY 
{
	vec2 mVelocity[];
};

layout (std430, binding = 5) buffer Color 
{
	vec4 mColor[];
};


void ComputeCounts(int gid);
ivec2 ComputeCellIndex(vec2 p);
void InsertBox(int gid);
int Index(int i, int j);
void CollisionQuery(int gid);
void Collide(int i, int j, aabb2D box_i, aabb2D box_j);
void Animate(int gid);

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
		InsertBox(gid);
	}
	if(mode==COLLISION_QUERY)
	{
		CollisionQuery(gid);
	}
   if(mode==ANIMATE)
	{
		Animate(gid);
	}
}

void ComputeCounts(int gid)
{
	ivec2 cell_min = ComputeCellIndex(mBoxes[gid].mMin);
	ivec2 cell_max = ComputeCellIndex(mBoxes[gid].mMax);

	for(int i=cell_min.x; i<=cell_max.x; i++)
	{
		for(int j=cell_min.y; j<=cell_max.y; j++)
		{
			int ix = Index(i,j);
			atomicAdd(mGridCounter[ix], 1);
		}
	}
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

void InsertBox(int gid)
{
   ivec2 cell_min = ComputeCellIndex(mBoxes[gid].mMin);
   ivec2 cell_max = ComputeCellIndex(mBoxes[gid].mMax);
   
   //reset color
   mColor[gid] = vec4(0.0);

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
}


void CollisionQuery(int gid)
{
	aabb2D query_aabb = mBoxes[gid];
	//These are the cells the query overlaps
   ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
   ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         int ix = Index(i,j);
         //Insert the list for each overlapped cell
         int offset = mGridOffset[ix];
         int count = mGridCounter[ix];
         for(int list_index = offset; list_index<offset+count; list_index++)
         {
            int box_index = mIndexList[list_index];
            aabb2D box = mBoxes[box_index];
            ivec2 home = max(cell_min, ComputeCellIndex(box.mMin));
            if(home == ivec2(i,j) && overlap(query_aabb, box))
            {
               Collide(gid, box_index, query_aabb, box);
            }
         }       
      }
   }
}


void Collide(int i, int j, aabb2D box_i, aabb2D box_j)
{
   if(i<=j) return;

   vec4 collide_color = vec4(1.0, 0.0, 0.0, 1.0);
   mColor[i] = collide_color;
   mColor[j] = collide_color;

   //*
   aabb2D isect = intersection(box_i, box_j); //overlap rectangle
   vec2 isect_size = isect.mMax-isect.mMin;
   int comp = isect_size.x < isect_size.y ? 0:1;
   float dist = 0.5f*isect_size[comp];

   vec2 cen_i = center(box_i);
   vec2 cen_j = center(box_j);

   vec2 dij = cen_j - cen_i; //points from i to j
   vec2 ud = normalize(dij);
   vec2 disp_j = vec2(0.0f);
   disp_j[comp] = sign(dij[comp])*dist;

   //unpenetrate
   //mBoxes[j].mMin += disp_j;
   //mBoxes[j].mMax += disp_j;
   atomicAdd(mBoxes[j].mMin.x, disp_j.x);
   atomicAdd(mBoxes[j].mMin.y, disp_j.y);

   atomicAdd(mBoxes[j].mMax.x, disp_j.x);
   atomicAdd(mBoxes[j].mMax.y, disp_j.y);
   
   //mBoxes[i].mMin -= disp_j;
   //mBoxes[i].mMax -= disp_j;
   atomicAdd(mBoxes[i].mMin.x, -disp_j.x);
   atomicAdd(mBoxes[i].mMin.y, -disp_j.y);

   atomicAdd(mBoxes[i].mMax.x, -disp_j.x);
   atomicAdd(mBoxes[i].mMax.y, -disp_j.y);

   float mi = area(mBoxes[i]);
   float mj = area(mBoxes[j]);
   float e = 0.75f;
   float f = -(1.0f+e)*dot(ud, mVelocity[i]-mVelocity[j])/(1.0f/mi+1.0f/mj);
   
   //mVelocity[j] -= f*ud; 
   //mVelocity[i] += f*ud; 

   atomicAdd(mVelocity[i].x, f*ud.x);
   atomicAdd(mVelocity[i].y, f*ud.y);

   atomicAdd(mVelocity[j].x, -f*ud.x);
   atomicAdd(mVelocity[j].y, -f*ud.y);
  // */
}


void Animate(int gid)
{
   float dt = 0.01f;
   vec2 vel = mVelocity[gid];
   aabb2D box = mBoxes[gid];

   vec2 d = vel*dt;

   box.mMin += d;
   box.mMax += d;

   //gravity
   vel.y -= 0.05*dt;

   //Collide with extents
   float e = 0.75f;
   for(int c=0; c<2; c++)
   {
      float overlap = mExtents.mMin[c] - box.mMin[c];
      if(overlap > 0.0f)
      {
         box.mMin[c] += overlap;
         box.mMax[c] += overlap;
         vel[c] *= -e;
      }

      overlap = box.mMax[c] - mExtents.mMax[c];
      if(overlap > 0.0f)
      {
         box.mMin[c] -= overlap;
         box.mMax[c] -= overlap;
         vel[c] *= -e;
      }
   }

   mVelocity[gid] = vel;
   mBoxes[gid] = box;
}
