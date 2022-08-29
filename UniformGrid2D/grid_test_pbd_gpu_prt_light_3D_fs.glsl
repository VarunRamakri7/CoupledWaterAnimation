#version 430

layout(location=1) uniform bool useAo = false;
layout(location=2) uniform float time = 0.0;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 M;
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

layout(std140, binding = 1) uniform LightUniforms
{
   vec4 La;	//ambient light color
   vec4 Ld;	//diffuse light color
   vec4 Ls;	//specular light color
   vec4 light_w; //world-space light position
};

layout(std140, binding = 2) uniform MaterialUniforms
{
   vec4 ka;	//ambient material color
   vec4 kd;	//diffuse material color
   vec4 ks;	//specular material color
   float shininess; //specular exponent
};


in VertexData
{
   vec4 pw;
   vec4 nw;
   vec4 no;
   vec4 color;
   vec4 po;
   vec4 p;
   flat int instanceId;
} inData;   

out vec4 fragcolor; //the output color for this fragment    

float ao_grid(vec3 pos, vec3 n);
vec3 prt_light(vec3 pos, vec3 n);

void main(void)
{   
   //fragcolor = inData.color;
   //return;

   //fragcolor = inData.pw;
   //return;

   vec4 ambient_term = ka*La;

   const float eps = 1e-8; //small value to avoid division by 0
   float d = distance(light_w.xyz, inData.pw.xyz);
   float atten = 1.0/(d*d + 1.01); //d-squared attenuation

   vec3 nw = normalize(inData.nw.xyz);			//world-space unit normal vector
   vec3 lw = normalize(light_w.xyz - inData.pw.xyz);	//world-space unit light vector
   vec4 diffuse_term = atten*kd*Ld*max(0.0, dot(nw, lw));

   vec3 vw = normalize(eye_w.xyz - inData.pw.xyz);	//world-space unit view vector
   vec3 rw = reflect(-lw, nw);	//world-space unit reflection vector

   vec4 specular_term = atten*ks*Ls*pow(max(0.0, dot(rw, vw)), shininess);

   float ao = 1.0;
   if(useAo == true) 
   {
      ao = ao_grid(inData.po.xyz, inData.no.xyz);
   }

   fragcolor = ao*ambient_term + diffuse_term + specular_term;
   fragcolor.xyz += 0.5*prt_light(inData.po.xyz, inData.no.xyz);
	
   //fragcolor = vec4(ao);
   fragcolor = pow(fragcolor, vec4(1.0/1.2));

   //bright boxes
   /*
   if(inData.instanceId>4000)
   {
      fragcolor = vec4(0.5, 0.75, 1.0, 1.0)*2.0;
   }
   */
}

struct aabb3D
{
   vec4 mMin;
   vec4 mMax;
};

layout(std140, binding = 3) uniform UniformGridInfo
{
	aabb3D mExtents;
	ivec4 mNumCells;
	vec4 mCellSize;
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

vec3 test_dir[26] = vec3[26](
	vec3(1.0, 0.0, 0.0), //face points
	vec3(-1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, -1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, -1.0),

	vec3(1.0, 1.0, 1.0), //vertex points
	vec3(-1.0, 1.0, 1.0),
	vec3(-1.0, -1.0, 1.0),
	vec3(1.0, -1.0, 1.0),
	vec3(1.0, 1.0, -1.0),
	vec3(-1.0, 1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(1.0, -1.0, -1.0),

	vec3(1.0, 1.0, 0.0), //edge points
	vec3(1.0, -1.0, 0.0),
	vec3(-1.0, -1.0, 0.0),
	vec3(-1.0, 1.0, 0.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 0.0, -1.0),
	vec3(-1.0, 0.0, -1.0),
	vec3(-1.0, 0.0, 1.0),
	vec3(0.0, 1.0, 1.0),
	vec3(0.0, 1.0, -1.0),
	vec3(0.0, -1.0, -1.0),
	vec3(0.0, -1.0, 1.0)
);

vec3 random3(vec3 c) {
    float j = 4096.0*sin(dot(c,vec3(13.0, 59.4, 15.0)));
    vec3 r;
    r.z = fract(512.0*j);
    j *= .125;
    r.x = fract(512.0*j);
    j *= .125;
    r.y = fract(512.0*j);
    return r-vec3(0.5);
}

bool collide_ray(vec3 pos, vec3 dir);
vec3 rayBoxIntersection(vec3 r0, vec3 rD, vec3 _min, vec3 _max);

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

float ao_grid(vec3 pos, vec3 n)
{
   pos += 0.001*n;
	float vis = 0.0;
	const int samples = 26;
   //const int samples = 1;
	for(int i=0; i<samples; i++)
	{
		//const float normal_bias = 0.5;
		//vec3 dir = normalize(test_dir[i].xyz) + normal_bias*n;
      vec3 dir = test_dir[i].xyz;
		const float rand_scale = 0.5;
		dir += rand_scale*random3(dir+pos);
		
		float ndotd = dot(n, dir);
		if(ndotd > 0.0)
		{
         dir = normalize(dir);

			bool hit = collide_ray(pos, dir);
			if(hit==false)
			{
				//vis += ndotd;
				vis += 1.0;
			}
		}

	}
	return vis/float(samples*0.5);
}

vec3 safeRd(vec3 d)
{
   vec3 rd = 1.0/d;
   //return rd;
   return clamp(rd, -1e30, 1e30);
/*
   const float eps = 1e-8;
   vec3 rd;
   rd.x = (d.x > eps) ? (1.0f / d.x) : 0.0f;
	rd.y = (d.y > eps) ? (1.0f / d.y) : 0.0f;
	rd.z = (d.z > eps) ? (1.0f / d.z) : 0.0f;
   return rd;
*/
}

bool collide_ray(vec3 pos, vec3 dir)
{
   //dir = dir-vec3(1e-6); //HACK
   //dir = normalize(test_dir[25]); //25, 24, 23, 22 failed
   

	const vec3 rCellSize = 1.0/mCellSize.xyz;

	const vec3 absDir = abs(dir);
	// for determining the fraction of the cell size the ray travels in each step:
	const vec3 rAbsDir = safeRd(absDir);

	// 1.1. Identify the voxel containing the ray origin.
	// these are integer coordinates:
	ivec3 start_cell = ComputeCellIndex(pos);

	// 1.2. Identify which coordinates are incremented/decremented as the ray crosses voxel boundaries.

	/// The integral steps in each primary direction when we iterate (1, 0 or -1)
	ivec3 iStep = ivec3(sign(dir));

	// 1.3. Determine the values of t at which the ray crosses the first voxel boundary along each dimension.
	const vec3 minXYZ = mExtents.mMin.xyz + start_cell*mCellSize.xyz;
	const vec3 maxXYZ = minXYZ + mCellSize.xyz;
	vec3 next_t; 
	for(int i=0;i<3; i++)
	{
		next_t[i] = ((iStep[i] >= 0) ? (maxXYZ[i] - pos[i]):(pos[i] - minXYZ[i]))*rAbsDir[i];
   }

	// 1.4. Compute how far we have to travel in the given direction (in units of t)
	// to reach the next voxel boundary (with each movement equal to the size of a cell) along each dimension.

	/// The t value which moves us from one voxel to the next
	vec3 delta_t = mCellSize.xyz*rAbsDir;
	
	ivec3 cell = start_cell;
   int safety = 0;
	for(;;)
	{
		if(any(lessThan(cell, ivec3(0)))) break;
		if(any(greaterThan(cell, mNumCells.xyz-ivec3(1)))) break;
      //if(safety > 150) break;
		
		//check for collisions in cell
		int ix = Index(cell.x, cell.y, cell.z);
		int offset = mGridOffset[ix];
      int count = mGridCounter[ix];
      
      for(int list_index = offset; list_index<offset+count; list_index++)
      {
         int box_index = mIndexList[list_index];
         //aabb3D box = aabb3D(mObj[box_index].x-mObj[box_index].hw, mObj[box_index].x+mObj[box_index].hw);

         vec3 box_min = mObj[box_index].x.xyz - mObj[box_index].hw.xyz;
         vec3 box_max = mObj[box_index].x.xyz + mObj[box_index].hw.xyz;
         vec3 isect = rayBoxIntersection(pos, dir, box_min, box_max);
         if(isect.z != 0.0)
         {
            return true;
         }
      }
		
		int min_comp = 0;
		if(next_t.y <= next_t.x && next_t.y <= next_t.z) min_comp = 1;
		if(next_t.z <= next_t.x && next_t.z <= next_t.y) min_comp = 2;
	
		cell[min_comp] += iStep[min_comp];
		next_t[min_comp] += delta_t[min_comp];
      safety++;
	}
	return false;
}


vec3 rayBoxIntersection(vec3 r0, vec3 rD, vec3 _min, vec3 _max)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    // inverse direction to catch float problems
    vec3 invrd = 1.0 / rD;

    if (invrd.x >= 0)
    {
        tmin = (_min.x - r0.x) * invrd.x;
        tmax = (_max.x - r0.x) * invrd.x;
    }else
    {
        tmin = (_max.x - r0.x) * invrd.x;
        tmax = (_min.x - r0.x) * invrd.x;
    }

    if (invrd.y >= 0)
    {
        tymin = (_min.y - r0.y) * invrd.y;
        tymax = (_max.y - r0.y) * invrd.y;
    }else
    {
        tymin = (_max.y - r0.y) * invrd.y;
        tymax = (_min.y - r0.y) * invrd.y;
    }

    if ( (tmin > tymax) || (tymin > tmax) ) return vec3(0.0);
    if ( tymin > tmin) tmin = tymin;
    if ( tymax < tmax) tmax = tymax;

    
    if (invrd.z >= 0)
    {
        tzmin = (_min.z - r0.z) * invrd.z;
        tzmax = (_max.z - r0.z) * invrd.z;
    }else
    {
        tzmin = (_max.z - r0.z) * invrd.z;
        tzmax = (_min.z - r0.z) * invrd.z;
    }

    if ( (tmin > tzmax) || (tzmin > tmax) ) return vec3(0.0);
    if ( tzmin > tmin) tmin = tzmin;
    if ( tzmax < tmax) tmax = tzmax;

    // check if values are valid
    if (tmin < 0) tmin = tmax;
    if (tmax < 0) return vec3(0.0);

    return vec3(tmin, tmax, 1.0);
}



////////////////////////////////////////////////////////////

layout(std140, binding = 10) uniform UniformPrtGridInfo
{
	aabb3D mExtents;
	ivec4 mNumCells;
	vec4 mCellSize;
} mParticleGridInfo;

layout (std430, binding = 10) buffer PRT_GRID_COUNTER 
{
	int mPrtGridCounter[];
};

layout (std430, binding = 11) buffer PRT_GRID_OFFSET 
{
	int mPrtGridOffset[];
};

layout (std430, binding = 12) buffer PRT_INDEX_LIST 
{
	int mPrtIndexList[];
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

ivec3 PrtComputeCellIndex(vec3 p)
{
   p = p-mParticleGridInfo.mExtents.mMin.xyz;
   ivec3 cell = ivec3(floor(p/mParticleGridInfo.mCellSize.xyz));
   cell = clamp(cell, ivec3(0), mParticleGridInfo.mNumCells.xyz-ivec3(1));
   return cell;
}

int PrtIndex(int i, int j, int k)
{
	return (i*mParticleGridInfo.mNumCells.y + j)*mParticleGridInfo.mNumCells.x + k;
}


vec3 prt_light(vec3 pos, vec3 n)
{
   /*
   //debug
   ivec3 cell = PrtComputeCellIndex(pos.xyz);
   int ix = PrtIndex(cell.x, cell.y, cell.z);
   int count = mPrtGridCounter[ix];
   return vec3(count/10.0);
   //*/

   vec3 diff = vec3(0.0);
   float r = 0.075; //search radius
   vec3 v = vec3(1.0)-abs(sign(n));
   aabb3D query_aabb = aabb3D(vec4(pos-r*v, 0.0), vec4(pos+r*(v+sign(n)), 0.0));
   //aabb3D query_aabb = aabb3D(vec4(pos-vec3(r), 0.0), vec4(pos+vec3(r), 0.0));

   //These are the cells the query overlaps
   ivec3 cell_min = PrtComputeCellIndex(query_aabb.mMin.xyz);
   ivec3 cell_max = PrtComputeCellIndex(query_aabb.mMax.xyz);

   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         for(int k=cell_min.z; k<=cell_max.z; k++)
         {
            int ix = PrtIndex(i,j,k);
            
            int offset = mPrtGridOffset[ix];
            int count = mPrtGridCounter[ix];
            for(int list_index = offset; list_index<offset+count; list_index++)
            {
               const float eps = 1e-8;
               int prt_index = mPrtIndexList[list_index];
               vec3 prt_pos = particles[prt_index].pos.xyz;
               float d = distance(prt_pos, pos)+eps;
               vec3 l = (prt_pos-pos)/d;
               float atten = 1.0/(2.0*d + 1.0)*smoothstep(r, 0.75*r, d);
               diff += atten*vec3(0.4, 0.175, 0.1)*max(0.0, dot(n, l));
            }
         }       
      }
   }
   return diff;
}