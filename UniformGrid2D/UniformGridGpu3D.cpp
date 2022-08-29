#include "UniformGridGpu3D.h"
#include "InitShader.h"
#include "imgui.h"

aabb3D::aabb3D()
{

}

aabb3D::aabb3D(glm::vec3 box_min, glm::vec3 box_max):
mMin(box_min, 0.0f), mMax(box_max, 0.0f)
{
   
}

UniformGridPbdGpu3D::UniformGridPbdGpu3D(glm::ivec4 num_cells, aabb3D extents)
{
   mUniformGridInfo.mNumCells = num_cells;
   mUniformGridInfo.mExtents = extents;
   mUniformGridInfo.mCellSize = (mUniformGridInfo.mExtents.mMax-mUniformGridInfo.mExtents.mMin)/glm::vec4(mUniformGridInfo.mNumCells);
}

void UniformGridPbdGpu3D::ClearCounter()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridCounter, offset, offset, size);
}

void UniformGridPbdGpu3D::ClearOffset()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridOffset, offset, offset, size);
}


void UniformGridPbdGpu3D::Init(std::vector<PbdObj>& obj)
{
   mParams.n = obj.size();
   mObjWorkgroups = glm::ceil(mParams.n/float(mMaxWorkGroup));

   if(mSsbo.mIndexList != -1)
   {
      glDeleteBuffers(1, &mSsbo.mIndexList);
   }

   GLuint flags = 0;
   glGenBuffers(1, &mSsbo.mIndexList);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mIndexList);
   glNamedBufferStorage(mSsbo.mIndexList, 8*sizeof(int)*obj.size(), nullptr, flags); //8 because each box can be in 8 cells
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   
   if(mSsbo.mObj != -1)
   {
      glDeleteBuffers(1, &mSsbo.mObj);
   }
   
   glGenBuffers(1, &mSsbo.mObj);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mObj);
   glNamedBufferStorage(mSsbo.mObj, sizeof(PbdObj)*obj.size(), obj.data(), flags); 
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mObj, mSsbo.mObj);
   
   if(mComputeShader != -1)
   {
      glDeleteProgram(mComputeShader);
   }
   mComputeShader = InitShader(mComputeShaderName.c_str());

   int num_cells = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z;
   std::vector<int> zeros(num_cells, 0);
   if(mSsbo.mZeros == -1)
   {
      glGenBuffers(1, &mSsbo.mZeros);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mZeros);
      glNamedBufferStorage(mSsbo.mZeros, sizeof(int)*zeros.size(), zeros.data(), flags);
   }

   if(mSsbo.mGridCounter == -1)
   {
      glGenBuffers(1, &mSsbo.mGridCounter);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridCounter);
      glNamedBufferStorage(mSsbo.mGridCounter, sizeof(int)*zeros.size(), zeros.data(), flags);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   }
   //Don't create: this should just be mParallelScan.mOutputSsbo
   //glGenBuffers(1, &mSsbo.mGridOffset);
   //glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridOffset);
   //glNamedBufferStorage(mSsbo.mGridOffset, sizeof(int)*zeros.size(), zeros.data(), flags);
   //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);

   if(mGridInfoUbo == -1)
   {
      glGenBuffers(1, &mGridInfoUbo);
      glBindBuffer(GL_UNIFORM_BUFFER, mGridInfoUbo);
      glNamedBufferStorage(mGridInfoUbo, sizeof(UniformGridInfo), &mUniformGridInfo, flags); 
      glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);
   }

   if(mParamsUbo == -1)
   {
      int flags = GL_DYNAMIC_STORAGE_BIT;
      glGenBuffers(1, &mParamsUbo);
      glBindBuffer(GL_UNIFORM_BUFFER, mParamsUbo);
      glNamedBufferStorage(mParamsUbo, sizeof(Params), &mParams, flags); 
      glBindBufferBase(GL_UNIFORM_BUFFER, mParamsBinding, mParamsUbo);
   }

   mParallelScan.Init(mSsbo.mGridCounter, mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z);
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;

   //TEST
   //collide_ray(glm::vec3(0.0f, 0.0f, 0.0f), glm::normalize(glm::vec3(0.0, 1.0, 1.0)));
}

void UniformGridPbdGpu3D::BufferParams()
{
   glNamedBufferSubData(mParamsUbo, 0, sizeof(Params), &mParams); 
}

void UniformGridPbdGpu3D::CollisionQuery()
{
   glUseProgram(mComputeShader);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mObj, mSsbo.mObj);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);
   glBindBufferBase(GL_UNIFORM_BUFFER, mParamsBinding, mParamsUbo);

   //clear counter and offsets
   ClearCounter();
   ClearOffset();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Compute Counts
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COMPUTE_COUNTS);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   

   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   glUseProgram(mComputeShader);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mObj, mSsbo.mObj);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);
   glBindBufferBase(GL_UNIFORM_BUFFER, mParamsBinding, mParamsUbo);

   ClearCounter();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Insert boxes into mIndexList
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.INSERT_BOXES);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //PreAnimate
   mUniforms.mTime += 0.1f;
   glProgramUniform1f(mComputeShader, mUniformLoc.mTime, mUniforms.mTime);
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.PREANIMATE);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Collision query
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   //for(int i=0; i<mParams.substeps; i++) //substeps here not working. In outer loop now
   {
      glDispatchCompute(mObjWorkgroups, 1, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   }

   //PostAnimate
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.POSTANIMATE);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


using namespace glm;

ivec3 UniformGridPbdGpu3D::ComputeCellIndex(vec3 p)
{
   p = p-vec3(mUniformGridInfo.mExtents.mMin);
   ivec3 cell = ivec3(floor(p/vec3(mUniformGridInfo.mCellSize)));
   cell = clamp(cell, ivec3(0), ivec3(mUniformGridInfo.mNumCells)-ivec3(1));
   return cell;
}

int UniformGridPbdGpu3D::Index(int i, int j, int k)
{
	return (i*mUniformGridInfo.mNumCells.y + j)*mUniformGridInfo.mNumCells.x + k;
}

vec3 safeRd(vec3 d)
{
    vec3 rd = 1.0f/d;
    return glm::clamp(rd, -1e30f, 1e30f);
/*
   const float eps = 1e-8f;
   vec3 rd;
   rd.x = (d.x > eps) ? (1.0f / d.x) : 0.0f;
	rd.y = (d.y > eps) ? (1.0f / d.y) : 0.0f;
	rd.z = (d.z > eps) ? (1.0f / d.z) : 0.0f;
   return rd;
*/
}

bool UniformGridPbdGpu3D::collide_ray(vec3 pos, vec3 dir)
{
	
	const vec3 rCellSize = 1.0f/vec3(mUniformGridInfo.mCellSize);

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
	const vec3 minXYZ = vec3(mUniformGridInfo.mExtents.mMin) + vec3(start_cell)*vec3(mUniformGridInfo.mCellSize);
	const vec3 maxXYZ = minXYZ + vec3(mUniformGridInfo.mCellSize);
	vec3 next_t; 
	for(int i=0;i<3; i++)
	{
		next_t[i] = ((iStep[i] >= 0) ? (maxXYZ[i] - pos[i]):(pos[i] - minXYZ[i]))*rAbsDir[i];
	}

	// 1.4. Compute how far we have to travel in the given direction (in units of t)
	// to reach the next voxel boundary (with each movement equal to the size of a cell) along each dimension.

	/// The t value which moves us from one voxel to the next
	vec3 delta_t = vec3(mUniformGridInfo.mCellSize)*rAbsDir;
	
	ivec3 cell = start_cell;
   int safety = 0;
	for(;;)
	{
		if(any(lessThan(cell, ivec3(0)))) break;
		if(any(greaterThan(cell, ivec3(mUniformGridInfo.mNumCells)-ivec3(1)))) break;
      if(safety > 200) break;
		
		int min_comp = 0;
		if(next_t.y <= next_t.x && next_t.y <= next_t.z) min_comp = 1;
		if(next_t.z <= next_t.x && next_t.z <= next_t.y) min_comp = 2;
	
		cell[min_comp] += iStep[min_comp];
		next_t[min_comp] += delta_t[min_comp];
      safety++;
	}
	return false;
}


#include <random>

GridTestPbdGpu3D::GridTestPbdGpu3D(int n):mGrid(glm::ivec4(16, 16, 16, 0), aabb3D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mNumObjects = n;
}


void GridTestPbdGpu3D::Init()
{
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.15f*mGrid.mUniformGridInfo.mCellSize.x, 0.45f*mGrid.mUniformGridInfo.mCellSize.x);

   std::vector<UniformGridPbdGpu3D::PbdObj> obj(mNumObjects);

   for(int i=0; i<obj.size(); i++)
   {
      glm::vec4 x = glm::vec4(0.5f*urand_pos(gen), 0.5f*urand_pos(gen), 0.5f*urand_pos(gen), 1.0f);
      glm::vec4 v = glm::vec4(urand_pos(gen), urand_pos(gen), urand_pos(gen), 0.0f);
      glm::vec4 h = glm::vec4(urand_size(gen), urand_size(gen), urand_size(gen), 0.0f);

/*
      float g = mGrid.mUniformGridInfo.mCellSize.x;
      float h1 = 0.15f*g;
      float h2 = 0.34f*g;
      glm::vec4 h = glm::vec4(h2, h2, h2, 0.0f);
      //h[i%3] = h2;
*/     
      
      obj[i].color = glm::vec4(1.0f);
      obj[i].x = x;
      obj[i].v = 0.1f*v;
      obj[i].p = x;
      obj[i].dp_t = glm::vec4(0.0f);
      obj[i].dp_n = glm::vec4(0.0f);
      obj[i].hw = h;
      obj[i].rm = 1.0f/(h.x*h.y*h.z);
      obj[i].n = 0.0f;
      obj[i].e = 1.0f;
   }

   mGrid.Init(obj);

   std::string vertex_shader = "grid_test_pbd_gpu_3D_vs.glsl";
   //std::string fragment_shader = "grid_test_pbd_gpu_3D_fs.glsl";
   std::string fragment_shader = "grid_test_pbd_gpu_prt_light_3D_fs.glsl";
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());
   glProgramUniform1i(mShader, mUniformLoc.mUseAo, mUniforms.mUseAo);

   glGenVertexArrays(1, &mAttriblessVao);
}

void GridTestPbdGpu3D::Draw()
{
   //Animate();

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mGrid.mSsboBinding.mIndexList, mGrid.mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mGrid.mSsboBinding.mObj, mGrid.mSsbo.mObj);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mGrid.mSsboBinding.mGridCounter, mGrid.mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mGrid.mSsboBinding.mGridOffset, mGrid.mSsbo.mGridOffset);
   
   int gridInfoBinding = 3;
   glBindBufferBase(GL_UNIFORM_BUFFER, gridInfoBinding, mGrid.mGridInfoUbo);
  
   glDrawArraysInstanced(GL_TRIANGLES, 0, 36, mNumObjects);

}

void GridTestPbdGpu3D::Animate(float t, float dt)
{
   mUniforms.mTime = t;
   glProgramUniform1f(mShader, mUniformLoc.mTime, t);
   for(int i=0; i<mGrid.mParams.substeps; i++)
   {
      mGrid.CollisionQuery();
   }
}

void GridTestPbdGpu3D::DrawGui()
{
   ImGui::Begin("GridTestPdb3D");
      if(ImGui::Button("Reinit"))
      {
         Init();
      }

      ImGui::SliderFloat2("g", &mGrid.mParams.g.x, -0.2f, +0.2f);
      ImGui::SliderFloat("dt", &mGrid.mParams.dt, 0.0f, +0.1f);
      ImGui::SliderFloat("damping", &mGrid.mParams.damping, 0.9f, 1.0f, "%0.4f");
      ImGui::SliderFloat("k (SOR)", &mGrid.mParams.k, 1.0f, 2.0f);
      ImGui::SliderFloat("k static", &mGrid.mParams.k_static, 1.0f, 10.0f);
      ImGui::SliderFloat("sleep thresh", &mGrid.mParams.sleep_thresh, 0.0f, +0.001f, "%0.4f");
      ImGui::SliderFloat("mu k", &mGrid.mParams.mu_k, 0.0f, 1.0f);
      ImGui::SliderFloat("mu s", &mGrid.mParams.mu_s, 0.0f, 1.0f);
      ImGui::SliderFloat("e", &mGrid.mParams.e, 0.0f, 1.0f);
      ImGui::SliderInt("substeps", &mGrid.mParams.substeps, 0, 20);

      if(ImGui::Button("Buffer Params"))
      {
         mGrid.BufferParams();
      }

      bool bUseAo = bool(mUniforms.mUseAo);
      if(ImGui::Checkbox("Ao", &bUseAo))
      {
         mUniforms.mUseAo = int(bUseAo);
         glProgramUniform1i(mShader, mUniformLoc.mUseAo, mUniforms.mUseAo);
      }
      
   ImGui::End();
}


