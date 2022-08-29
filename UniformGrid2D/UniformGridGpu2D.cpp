#include "UniformGridGpu2D.h"
#include "InitShader.h"

UniformGridGpu2D::UniformGridGpu2D(glm::ivec2 num_cells, aabb2D extents)
{
   mUniformGridInfo.mNumCells = num_cells;
   mUniformGridInfo.mExtents = extents;
   mUniformGridInfo.mCellSize = (mUniformGridInfo.mExtents.mMax-mUniformGridInfo.mExtents.mMin)/glm::vec2(mUniformGridInfo.mNumCells);
}

void UniformGridGpu2D::ClearCounter()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridCounter, offset, offset, size);
}

void UniformGridGpu2D::ClearOffset()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridOffset, offset, offset, size);
}


void UniformGridGpu2D::Init(std::vector<aabb2D>& boxes)
{
   mBoxes = boxes;
   mBoxWorkgroups = glm::ceil(mBoxes.size()/float(mMaxWorkGroup));
   if(mSsbo.mIndexList == -1)
   {
      GLuint flags = 0;
      glGenBuffers(1, &mSsbo.mIndexList);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mIndexList);
      glNamedBufferStorage(mSsbo.mIndexList, 4*sizeof(int)*boxes.size(), nullptr, flags); //4 because each box can be in 4 cells
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   }

   if(mSsbo.mBoxes == -1)
   {
      GLuint flags = 0;
      glGenBuffers(1, &mSsbo.mBoxes);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mBoxes);
      glNamedBufferStorage(mSsbo.mBoxes, sizeof(aabb2D)*boxes.size(), boxes.data(), flags); 
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);

      glGenBuffers(1, &mSsbo.mVelocity);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mVelocity);
      glNamedBufferStorage(mSsbo.mVelocity, sizeof(glm::vec2)*boxes.size(), nullptr, flags);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mVelocity, mSsbo.mVelocity);
   
      glGenBuffers(1, &mSsbo.mColor);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mColor);
      glNamedBufferStorage(mSsbo.mColor, sizeof(glm::vec4)*boxes.size(), nullptr, flags);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mColor, mSsbo.mColor);
   }

   mComputeShader = InitShader(mComputeShaderName.c_str());
   glProgramUniform1i(mComputeShader, mUniformLoc.mNumBoxes, boxes.size()); 

   std::vector<int> zeros(mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y, 0);
   GLuint flags = 0;
   glGenBuffers(1, &mSsbo.mZeros);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mZeros);
   glNamedBufferStorage(mSsbo.mZeros, sizeof(int)*zeros.size(), zeros.data(), flags);

   glGenBuffers(1, &mSsbo.mGridCounter);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridCounter);
   glNamedBufferStorage(mSsbo.mGridCounter, sizeof(int)*zeros.size(), zeros.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);

   //TODO: this should just be mParallelScan.mOutputSsbo
   glGenBuffers(1, &mSsbo.mGridOffset);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridOffset);
   glNamedBufferStorage(mSsbo.mGridOffset, sizeof(int)*zeros.size(), zeros.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);

   glGenBuffers(1, &mGridInfoUbo);
   glBindBuffer(GL_UNIFORM_BUFFER, mGridInfoUbo);
   glNamedBufferStorage(mGridInfoUbo, sizeof(UniformGridInfo), &mUniformGridInfo, flags); 
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   mParallelScan.Init(mSsbo.mGridCounter, mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y);
}

void UniformGridGpu2D::CollisionQuery()
{
   glUseProgram(mComputeShader);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mVelocity, mSsbo.mVelocity);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   //clear counter and offsets
   ClearCounter();
   ClearOffset();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Compute Counts
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COMPUTE_COUNTS);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   
   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   glUseProgram(mComputeShader);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mVelocity, mSsbo.mVelocity);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   ClearCounter();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Insert boxes into mIndexList
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.INSERT_BOXES);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Collision query
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Animate
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.ANIMATE);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}












#include <random>
GridTestGpuAnimate::GridTestGpuAnimate(int n):mGrid(glm::ivec2(32, 32), aabb2D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mBoxes.resize(n);
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.002f, 0.01f);

   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 cen = glm::vec2(0.9f*urand_pos(gen), 0.9f*urand_pos(gen));
      glm::vec2 h = glm::vec2(urand_size(gen), urand_size(gen));
      mBoxes[i] = aabb2D(cen-h, cen+h);
   }

   mVelocity.resize(n);
   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 vel = glm::vec2(urand_pos(gen), urand_pos(gen));
      mVelocity[i] = 0.1f*vel;
   }

   //mBoxes[n-1].mMin = glm::vec2(-0.125f);
   //mBoxes[n-1].mMax = glm::vec2(+0.125f);
   //mVelocity[n-1] = glm::vec2(0.0f);
}


void GridTestGpuAnimate::Init()
{
   std::string vertex_shader = "grid_test_gpu_vs.glsl";
   std::string fragment_shader = "grid_test_gpu_fs.glsl";
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   std::vector<int> all_indices(mBoxes.size());
   for(int i=0; i<mBoxes.size(); i++)
   {
      all_indices[i] = i;
   }

   glGenVertexArrays(1, &mAttriblessVao);

   mGrid.Init(mBoxes);

   mSsbo.mBoxes = mGrid.mSsbo.mBoxes;
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);

   mSsbo.mColors = mGrid.mSsbo.mColor;
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mColors, mSsbo.mColors);

   
}

void GridTestGpuAnimate::Draw()
{
   Animate();

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glDisable(GL_DEPTH_TEST);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mColors, mSsbo.mColors);
  
   glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mBoxes.size());

   glEnable(GL_DEPTH_TEST);
}

void GridTestGpuAnimate::Animate(float t, float dt)
{
   mGrid.CollisionQuery();
}

















UniformGridPbdGpu2D::UniformGridPbdGpu2D(glm::ivec2 num_cells, aabb2D extents)
{
   mUniformGridInfo.mNumCells = num_cells;
   mUniformGridInfo.mExtents = extents;
   mUniformGridInfo.mCellSize = (mUniformGridInfo.mExtents.mMax-mUniformGridInfo.mExtents.mMin)/glm::vec2(mUniformGridInfo.mNumCells);
}

void UniformGridPbdGpu2D::ClearCounter()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridCounter, offset, offset, size);
}

void UniformGridPbdGpu2D::ClearOffset()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridOffset, offset, offset, size);
}


void UniformGridPbdGpu2D::Init(std::vector<PbdObj>& obj)
{
   mParams.n = obj.size();
   mObjWorkgroups = glm::ceil(mParams.n/float(mMaxWorkGroup));
   if(mSsbo.mIndexList == -1)
   {
      GLuint flags = 0;
      glGenBuffers(1, &mSsbo.mIndexList);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mIndexList);
      glNamedBufferStorage(mSsbo.mIndexList, 4*sizeof(int)*obj.size(), nullptr, flags); //4 because each box can be in 4 cells
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   }

   if(mSsbo.mObj == -1)
   {
      GLuint flags = 0;
      glGenBuffers(1, &mSsbo.mObj);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mObj);
      glNamedBufferStorage(mSsbo.mObj, sizeof(PbdObj)*obj.size(), obj.data(), flags); 
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mObj, mSsbo.mObj);
   }

   mComputeShader = InitShader(mComputeShaderName.c_str());

   std::vector<int> zeros(mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y, 0);
   GLuint flags = 0;
   glGenBuffers(1, &mSsbo.mZeros);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mZeros);
   glNamedBufferStorage(mSsbo.mZeros, sizeof(int)*zeros.size(), zeros.data(), flags);

   glGenBuffers(1, &mSsbo.mGridCounter);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridCounter);
   glNamedBufferStorage(mSsbo.mGridCounter, sizeof(int)*zeros.size(), zeros.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);

   //Don't create: this should just be mParallelScan.mOutputSsbo
   //glGenBuffers(1, &mSsbo.mGridOffset);
   //glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridOffset);
   //glNamedBufferStorage(mSsbo.mGridOffset, sizeof(int)*zeros.size(), zeros.data(), flags);
   //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);

   glGenBuffers(1, &mGridInfoUbo);
   glBindBuffer(GL_UNIFORM_BUFFER, mGridInfoUbo);
   glNamedBufferStorage(mGridInfoUbo, sizeof(UniformGridInfo), &mUniformGridInfo, flags); 
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   glGenBuffers(1, &mParamsUbo);
   glBindBuffer(GL_UNIFORM_BUFFER, mParamsUbo);
   glNamedBufferStorage(mParamsUbo, sizeof(Params), &mParams, flags); 
   glBindBufferBase(GL_UNIFORM_BUFFER, mParamsBinding, mParamsUbo);

   mParallelScan.Init(mSsbo.mGridCounter, mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y);
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;
}

void UniformGridPbdGpu2D::CollisionQuery()
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
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.PREANIMATE);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Collision query
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   for(int i=0; i<mParams.substeps; i++)
   {
      glDispatchCompute(mObjWorkgroups, 1, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   }

   //PostAnimate
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.POSTANIMATE);
   glDispatchCompute(mObjWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
















GridTestPbdGpu2D::GridTestPbdGpu2D(int n):mGrid(glm::ivec2(32, 32), aabb2D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mNumObjects = n;
}


void GridTestPbdGpu2D::Init()
{
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.002f, 0.01f);

   std::vector<UniformGridPbdGpu2D::PbdObj> obj(mNumObjects);

   for(int i=0; i<obj.size(); i++)
   {
      glm::vec2 x = glm::vec2(0.5f*urand_pos(gen), 0.5f*urand_pos(gen));
      glm::vec2 v = glm::vec2(urand_pos(gen), urand_pos(gen));
      glm::vec2 h = glm::vec2(urand_size(gen), urand_size(gen));
      
      obj[i].color = glm::vec4(1.0f);
      obj[i].x = x;
      obj[i].v = 0.1f*v;
      obj[i].p = x;
      obj[i].dp_t = glm::vec2(0.0f);
      obj[i].dp_n = glm::vec2(0.0f);
      obj[i].hw = h;
      obj[i].rm = 1.0f/(h.x*h.y);
      obj[i].n = 0.0f;
      obj[i].e = 1.0f;
   }

   obj[mNumObjects-1].hw = glm::vec2(0.06f);
   obj[mNumObjects-1].rm = 1.0f/(obj[mNumObjects-1].hw.x*obj[mNumObjects-1].hw.y);

   mGrid.Init(obj);

   std::string vertex_shader = "grid_test_pbd_gpu_vs.glsl";
   std::string fragment_shader = "grid_test_pbd_gpu_fs.glsl";
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   glGenVertexArrays(1, &mAttriblessVao);
}

void GridTestPbdGpu2D::Draw()
{
   Animate();

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glDisable(GL_DEPTH_TEST);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mGrid.mSsboBinding.mObj, mGrid.mSsbo.mObj);
  
   glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mNumObjects);

   glEnable(GL_DEPTH_TEST);
}

void GridTestPbdGpu2D::Animate(float t, float dt)
{
   mGrid.CollisionQuery();
}