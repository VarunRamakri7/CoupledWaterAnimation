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
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Compute Counts
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COMPUTE_COUNTS);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
   
   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   glUseProgram(mComputeShader);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mBoxes, mSsbo.mBoxes);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mVelocity, mSsbo.mVelocity);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   ClearCounter();
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Insert boxes into mIndexList
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.INSERT_BOXES);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Collision query
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Animate
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.ANIMATE);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}







///////////////////////////////////////////////////////////////////////










UniformGridSph2D::UniformGridSph2D(glm::ivec2 num_cells, aabb2D extents)
{
   mUniformGridInfo.mNumCells = num_cells;
   mUniformGridInfo.mExtents = extents;
   mUniformGridInfo.mCellSize = (mUniformGridInfo.mExtents.mMax - mUniformGridInfo.mExtents.mMin) / glm::vec2(mUniformGridInfo.mNumCells);
}

void UniformGridSph2D::ClearCounter()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x * mUniformGridInfo.mNumCells.y * sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridCounter, offset, offset, size);
}

void UniformGridSph2D::ClearOffset()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x * mUniformGridInfo.mNumCells.y * sizeof(int);
   glCopyNamedBufferSubData(mSsbo.mZeros, mSsbo.mGridOffset, offset, offset, size);
}


void UniformGridSph2D::Init(GLuint particles_ssbo, GLuint particles_binding, int num_particles)
{
   mSsbo.mParticles = particles_ssbo;
   mSsboBinding.mParticles = particles_binding;
   mBoxWorkgroups = glm::ceil(num_particles / float(mMaxWorkGroup));
   if (mSsbo.mIndexList == -1)
   {
      GLuint flags = 0;
      glGenBuffers(1, &mSsbo.mIndexList);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mIndexList);
      glNamedBufferStorage(mSsbo.mIndexList, 4 * sizeof(int) * num_particles, nullptr, flags); //4 because each box can be in 4 cells
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   }

   mComputeShader = InitShader(mComputeShaderName.c_str());
   glProgramUniform1i(mComputeShader, mUniformLoc.mNumBoxes, num_particles);

   std::vector<int> zeros(mUniformGridInfo.mNumCells.x * mUniformGridInfo.mNumCells.y, 0);
   GLuint flags = 0;
   glGenBuffers(1, &mSsbo.mZeros);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mZeros);
   glNamedBufferStorage(mSsbo.mZeros, sizeof(int) * zeros.size(), zeros.data(), flags);

   glGenBuffers(1, &mSsbo.mGridCounter);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridCounter);
   glNamedBufferStorage(mSsbo.mGridCounter, sizeof(int) * zeros.size(), zeros.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);

   //TODO: this should just be mParallelScan.mOutputSsbo
   glGenBuffers(1, &mSsbo.mGridOffset);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSsbo.mGridOffset);
   glNamedBufferStorage(mSsbo.mGridOffset, sizeof(int) * zeros.size(), zeros.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);

   glGenBuffers(1, &mGridInfoUbo);
   glBindBuffer(GL_UNIFORM_BUFFER, mGridInfoUbo);
   glNamedBufferStorage(mGridInfoUbo, sizeof(UniformGridInfo), &mUniformGridInfo, flags);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   mParallelScan.Init(mSsbo.mGridCounter, mUniformGridInfo.mNumCells.x * mUniformGridInfo.mNumCells.y);
}

void UniformGridSph2D::CollisionQuery()
{
   glUseProgram(mComputeShader);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mParticles, mSsbo.mParticles);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   //clear counter and offsets
   ClearCounter();
   ClearOffset();
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Compute Counts
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COMPUTE_COUNTS);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mSsbo.mGridOffset = mParallelScan.mOutputSsbo;
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   glUseProgram(mComputeShader);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mParticles, mSsbo.mParticles);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, mSsbo.mGridOffset);
   glBindBufferBase(GL_UNIFORM_BUFFER, mGridInfoBinding, mGridInfoUbo);

   ClearCounter();
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Insert boxes into mIndexList
   glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.INSERT_BOXES);
   glDispatchCompute(mBoxWorkgroups, 1, 1);
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   //Collision query
   //glProgramUniform1i(mComputeShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   //glDispatchCompute(mBoxWorkgroups, 1, 1);
   //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

}