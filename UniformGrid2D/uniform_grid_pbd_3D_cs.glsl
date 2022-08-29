#version 450
#extension GL_NV_shader_atomic_float : enable

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

/*
vec2 center(aabb2D a)
{
   return 0.5*(a.mMin + a.mMax);
}

float area(aabb2D box)
{
   vec2 diff = box.mMax-box.mMin;
   return diff.x*diff.y;
}
*/

//Modes
const int COMPUTE_COUNTS = 0;
const int COMPUTE_OFFSETS = 1;
const int INSERT_BOXES = 2;
const int PREANIMATE = 3;
const int COLLISION_QUERY = 4;
const int POSTANIMATE = 5;

layout(location=0) uniform int mode = COMPUTE_COUNTS;
layout(location=1) uniform float time = 0.0;

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

layout (std430, binding = 3) buffer OBJS 
{
	PbdObj mObj[];
};

void ComputeCounts(int gid);
ivec3 ComputeCellIndex(vec3 p);
void InsertBox(int gid);
int Index(int i, int j, int k);
void CollisionQuery(int gid);
void Collide(int i, int j, aabb3D box_i, aabb3D box_j);
void PreAnimate(int gid);
void PostAnimate(int gid);

void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	if(gid >= mParams.n) return;

	if(mode==COMPUTE_COUNTS)
	{
		ComputeCounts(gid);	
	}
	if(mode==INSERT_BOXES)
	{
		InsertBox(gid);
	}
   if(mode==PREANIMATE)
	{
		PreAnimate(gid);
	}
	if(mode==COLLISION_QUERY)
	{
		CollisionQuery(gid);
	}
   if(mode==POSTANIMATE)
	{
		PostAnimate(gid);
	}
}

void ComputeCounts(int gid)
{
   //aabb3D box = aabb3D(mObj[gid].x-mObj[gid].hw, mObj[gid].x+mObj[gid].hw);
	//ivec3 cell_min = ComputeCellIndex(box.mMin.xyz);
	//ivec3 cell_max = ComputeCellIndex(box.mMax.xyz);

   ivec3 cell_min = ComputeCellIndex(mObj[gid].x.xyz - mObj[gid].hw.xyz);
	ivec3 cell_max = ComputeCellIndex(mObj[gid].x.xyz + mObj[gid].hw.xyz);

	for(int i=cell_min.x; i<=cell_max.x; i++)
	{
		for(int j=cell_min.y; j<=cell_max.y; j++)
		{
         for(int k=cell_min.z; k<=cell_max.z; k++)
		   {
			   int ix = Index(i,j,k);
			   atomicAdd(mGridCounter[ix], 1);
         }
		}
	}
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

void InsertBox(int gid)
{
   //aabb3D box = aabb3D(mObj[gid].x-mObj[gid].hw, mObj[gid].x+mObj[gid].hw);
   //ivec3 cell_min = ComputeCellIndex(box.mMin.xyz);
   //ivec3 cell_max = ComputeCellIndex(box.mMax.xyz);

   ivec3 cell_min = ComputeCellIndex(mObj[gid].x.xyz - mObj[gid].hw.xyz);
   ivec3 cell_max = ComputeCellIndex(mObj[gid].x.xyz + mObj[gid].hw.xyz);
   
   //reset color
   mObj[gid].color = vec4(1.0);

   //insert box into each cell it overlaps
   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         for(int k=cell_min.z; k<=cell_max.z; k++)
         {
			   int ix = Index(i,j,k);
            int offset = mGridOffset[ix];
            int count = atomicAdd(mGridCounter[ix], 1);
            mIndexList[offset+count] = gid;
         }
      }
   }
}


void CollisionQuery(int gid)
{
	aabb3D query_aabb = aabb3D(mObj[gid].p-mObj[gid].hw, mObj[gid].p+mObj[gid].hw);
   
   //Collide with extents
   for(int c = 0; c<3; c++) //dimension: x, y, or z
   {
      float overlap[2] = {mExtents.mMin[c]-query_aabb.mMin[c], query_aabb.mMax[c] - mExtents.mMax[c]};
      float dir[2] = {+1.0f, -1.0f};
      for(int m=0; m<2; m++) //min or max
      {
         if(overlap[m] > 0.0f)
         {
            mObj[gid].dp_n[c] += dir[m]*mParams.k_static*overlap[m];
            mObj[gid].n += mParams.k_static;

            vec4 dp_perp = mObj[gid].x-mObj[gid].p;
            dp_perp[c] = 0.0;
            vec4 frict_j = dp_perp;
            float dp_perp_mag = length(dp_perp.xyz);
            if(dp_perp_mag < mParams.mu_s*overlap[m])
            {
               frict_j = frict_j*min(1.0f, mParams.mu_k*overlap[m]/dp_perp_mag);
            }
            //mObj[gid].dp_t += frict_j;
            atomicAdd(mObj[gid].dp_t.x, frict_j.x);
            atomicAdd(mObj[gid].dp_t.y, frict_j.y);
            atomicAdd(mObj[gid].dp_t.z, frict_j.z);
         }
      }
   }

	//These are the cells the query overlaps
   ivec3 cell_min = ComputeCellIndex(query_aabb.mMin.xyz);
   ivec3 cell_max = ComputeCellIndex(query_aabb.mMax.xyz);

   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         for(int k=cell_min.z; k<=cell_max.z; k++)
         {
            int ix = Index(i,j,k);
            //Insert the list for each overlapped cell
            int offset = mGridOffset[ix];
            int count = mGridCounter[ix];
            for(int list_index = offset; list_index<offset+count; list_index++)
            {
               int box_index = mIndexList[list_index];
               aabb3D box = aabb3D(mObj[box_index].x-mObj[box_index].hw, mObj[box_index].x+mObj[box_index].hw);
               ivec3 home = max(cell_min, ComputeCellIndex(box.mMin.xyz));
               if(home == ivec3(i,j,k))
               {
                  Collide(gid, box_index, query_aabb, box);
               }
            }
         }       
      }
   }
}


void Collide(int i, int j, aabb3D box_i, aabb3D box_j)
{
   if(i>=j) return;

   //*
   aabb3D isect = intersection(box_i, box_j); //overlap rectangle
   vec3 isect_size = isect.mMax.xyz-isect.mMin.xyz;
   //int comp = isect_size.x < isect_size.y ? 0:1;
   int comp = 0;
   if(isect_size.y < isect_size.x && isect_size.y < isect_size.z) comp = 1;
   if(isect_size.z < isect_size.x && isect_size.z < isect_size.y) comp = 2;

   float dist = 0.5f*isect_size[comp];
   if(dist < 0.0) return;

   vec4 collide_color = vec4(1.0, 0.0, 0.0, 1.0);
   mObj[i].color = collide_color;
   mObj[j].color = collide_color;

   vec4 cen_i = mObj[i].p;
   vec4 cen_j = mObj[j].p;

   float eps = 1e-8f;
   vec4 dij = cen_j - cen_i; //points from i to j
   vec4 ud = dij/(length(dij.xyz)+eps);
   vec4 disp_j = vec4(0.0f);
   disp_j[comp] = sign(dij[comp])*dist;

   const float rmi = mObj[i].rm;
   const float rmj = mObj[j].rm;
   const float wi = rmi/(rmi+rmj);
   const float wj = rmj/(rmi+rmj);

   //mObj[j].dp_n += wj*disp_j;
   //mObj[j].n += 1.0f;
   atomicAdd(mObj[j].dp_n.x, wj*disp_j.x);
   atomicAdd(mObj[j].dp_n.y, wj*disp_j.y);
   atomicAdd(mObj[j].dp_n.z, wj*disp_j.z);
   atomicAdd(mObj[j].n, 1.0);

   //mObj[i].dp_n -= wi*disp_j;
   //mObj[i].n += 1.0f;
   atomicAdd(mObj[i].dp_n.x, -wi*disp_j.x);
   atomicAdd(mObj[i].dp_n.y, -wi*disp_j.y);
   atomicAdd(mObj[i].dp_n.z, -wi*disp_j.z);
   atomicAdd(mObj[i].n, 1.0);

   //friction
   vec4 dp_perp = ((mObj[j].x-cen_j)-(mObj[i].x-cen_i));
   dp_perp = dp_perp - dot(dp_perp.xyz, ud.xyz)*ud;

   vec4 frict_j = dp_perp;
   float dp_perp_mag = length(dp_perp.xyz);
   if(dp_perp_mag < mParams.mu_s*dist)
   {
      frict_j = frict_j*min(1.0f, mParams.mu_k*dist/dp_perp_mag);
   }

   //mObj[j].dp_t += wj*frict_j;
   //mObj[i].dp_t -= wi*frict_j;
   atomicAdd(mObj[j].dp_t.x, wj*frict_j.x);
   atomicAdd(mObj[j].dp_t.y, wj*frict_j.y);
   atomicAdd(mObj[j].dp_t.z, wj*frict_j.z);
   atomicAdd(mObj[i].dp_t.x, -wi*frict_j.x);
   atomicAdd(mObj[i].dp_t.y, -wi*frict_j.y);
   atomicAdd(mObj[i].dp_t.z, -wi*frict_j.z);
}


void PreAnimate(int i)
{
   //Apply external forces, damping
   vec4 v = mParams.damping*(mObj[i].v + mParams.dt*mParams.g);

   //Zero out the accumulators
   mObj[i].dp_n = vec4(0.0); 
   mObj[i].dp_t = vec4(0.0); //displacements from constraints
   mObj[i].n = 0.0; //number of constraints applied

   //Compute estimates
   mObj[i].p = mObj[i].x + mParams.dt*v;
   mObj[i].v = v;

/*
*  //change size
   if(i>mParams.n/2)
   {
      vec3 hw0 = mObj[i-mParams.n/2].hw.xyz;
      mObj[i].hw.xyz = hw0*(0.5*sin(time + 0.1*i)+0.55);
   }
*/
}

void PostAnimate(int i)
{
   vec4 p = mObj[i].p;
   vec4 pe = p;
   const float n = mObj[i].n;
   if(n > 0.0f)
   {
      p += mParams.k*(mObj[i].dp_t + mObj[i].dp_n)/n;
      pe += mParams.k*(mObj[i].dp_t + mParams.e*mObj[i].dp_n)/n;
      mObj[i].p = p;
   }
   //mObj[i].v = (mObj[i].p - mObj[i].x)/mParams.dt;
   mObj[i].v = (pe - mObj[i].x)/mParams.dt;

   //sleeping
   if(length(mObj[i].x.xyz - p.xyz) > mParams.sleep_thresh)
   {
      mObj[i].x = p;
   }
}