#include "UniformGridParticles3D.h"
#include "InitShader.h"

ComputeShader::ComputeShader(std::string filename):mFilename(filename)
{

}

void ComputeShader::Init()
{
   if(mShader != -1)
   {
      glDeleteProgram(mShader);
   }
   mShader = InitShader(mFilename.c_str());
}

void ComputeShader::SetSize(glm::ivec3 size)
{
   mSize = size;
   mNumWorkgroups = glm::ceil(glm::vec3(mSize)/glm::vec3(mMaxWorkGroup));
   mNumWorkgroups = glm::max(mNumWorkgroups, glm::ivec3(1));
}

void ComputeShader::Dispatch()
{
   glDispatchCompute(mNumWorkgroups.x, mNumWorkgroups.y, mNumWorkgroups.z);
}



Buffer::Buffer(GLuint target, GLuint binding): mTarget(target), mBinding(binding)
{

}

void Buffer::Init(int size, void* data, GLuint flags)
{
   mSize = size;
   mFlags = flags;
   if(mEnableDebug == true)
   {
      mFlags = mFlags | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;
   }
   
   if(mBuffer != -1)
   {
      glDeleteBuffers(1, &mBuffer);
   }

   glGenBuffers(1, &mBuffer);
   glBindBuffer(mTarget, mBuffer);
   glNamedBufferStorage(mBuffer, size, data, mFlags);
}

void Buffer::BindBufferBase()
{
   glBindBufferBase(mTarget, mBinding, mBuffer);
}

void Buffer::DebugReadInt()
{
   if(mEnableDebug)
   {
      glBindBuffer(mTarget, mBuffer);
      int *ptr = (int*) glMapNamedBuffer(mBuffer, GL_READ_ONLY);
      std::vector<int> test;
      for (int i = 0; i < mSize/sizeof(int); i++)
      {
         test.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
}

void Buffer::DebugReadFloat()
{
   if(mEnableDebug)
   {
      glBindBuffer(mTarget, mBuffer);
      float *ptr = (float*) glMapNamedBuffer(mBuffer, GL_READ_ONLY);
      std::vector<float> test;
      for (int i = 0; i < mSize/sizeof(float); i++)
      {
         test.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
}


Ugrid3D::Ugrid3D(glm::ivec4 num_cells, aabb3D extents)
{
   mUniformGridInfo.mNumCells = num_cells;
   mUniformGridInfo.mExtents = extents;
   mUniformGridInfo.mCellSize = (mUniformGridInfo.mExtents.mMax-mUniformGridInfo.mExtents.mMin)/glm::vec4(mUniformGridInfo.mNumCells);
}

void Ugrid3D::ClearCounter()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z*sizeof(int);
   glCopyNamedBufferSubData(mZerosSsbo.mBuffer, mGridCounterSsbo.mBuffer, offset, offset, size);
}

void Ugrid3D::ClearOffset()
{
   const int offset = 0;
   const int size = mUniformGridInfo.mNumCells.x*mUniformGridInfo.mNumCells.y*mUniformGridInfo.mNumCells.z*sizeof(int);
   glCopyNamedBufferSubData(mZerosSsbo.mBuffer, mGridOffsetSsbo.mBuffer, offset, offset, size);
}

void Ugrid3D::Init()
{
   mComputeShader.Init();

   int flags = 0;
   int num_cells = mUniformGridInfo.num_cells();
   std::vector<int> zeros(num_cells, 0);

   mZerosSsbo.Init(sizeof(int)*zeros.size(), zeros.data(), flags);
   mGridCounterSsbo.Init(sizeof(int)*zeros.size(), zeros.data(), flags);
   
   mGridInfoUbo.Init(sizeof(UniformGridInfo), &mUniformGridInfo, flags);

   mParallelScan.Init(mGridCounterSsbo.mBuffer, num_cells);
   mGridOffsetSsbo.mBuffer = mParallelScan.mOutputSsbo;
}

void Ugrid3D::ComputeOffsets()
{
   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mGridOffsetSsbo.mBuffer = mParallelScan.mOutputSsbo;
}


UgridParticles3D::UgridParticles3D(glm::ivec4 num_cells, aabb3D extents) : Ugrid3D(num_cells, extents)
{

}

void UgridParticles3D::Init(std::vector<Particle>& particles)
{
   Ugrid3D::Init();  

   mUniforms.mNumParticles = particles.size();
   glProgramUniform1i(mComputeShader.mShader, mUniformLoc.mNumParticles, mUniforms.mNumParticles);
   mParticleWorkgroups = glm::ceil(glm::vec3(mUniforms.mNumParticles, 0, 0)/glm::vec3(mComputeShader.GetMaxWorkGroup()));

   GLuint flags = 0;
   mIndexListSsbo.Init(1*sizeof(int)*particles.size(), nullptr, flags); //1* because each particle can only be in a single cell
   mParticleSsbo.Init(sizeof(Particle)*particles.size(), particles.data(), flags); 
}

void UgridParticles3D::Init(GLuint particleSsbo, int num_particles)
{
   Ugrid3D::Init();  

   mUniforms.mNumParticles = num_particles; 
   glProgramUniform1i(mComputeShader.mShader, mUniformLoc.mNumParticles, mUniforms.mNumParticles);
   mParticleWorkgroups = glm::ceil(glm::vec3(mUniforms.mNumParticles, 1, 1)/glm::vec3(mComputeShader.GetMaxWorkGroup())); 
   
   GLuint flags = 0;
   mIndexListSsbo.Init(1*sizeof(int)*mUniforms.mNumParticles, nullptr, flags);
   mParticleSsbo.mBuffer = particleSsbo;
   mParticleSsbo.mSize = num_particles*sizeof(Particle);
}

void UgridParticles3D::BuildGrid()
{
   //clear counter and offsets
   ClearCounter();
   ClearOffset();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   mGridCounterSsbo.BindBufferBase();
   mGridOffsetSsbo.BindBufferBase();
   mGridInfoUbo.BindBufferBase();
   mIndexListSsbo.BindBufferBase();
   mParticleSsbo.BindBufferBase();
  
   //Compute Counts
   glUseProgram(mComputeShader.mShader);
   glProgramUniform1i(mComputeShader.mShader, Ugrid3D::mUniformLoc.mMode, Ugrid3D::mUniforms.COMPUTE_COUNTS);
   glDispatchCompute(mParticleWorkgroups.x, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);  


   //Compute Offsets (exclusive prefix sum of counts)
   mParallelScan.Compute();
   mGridOffsetSsbo.mBuffer = mParallelScan.mOutputSsbo;
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


   ClearCounter();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   //Insert boxes into mIndexList
   glUseProgram(mComputeShader.mShader);
   mGridCounterSsbo.BindBufferBase();
   mGridOffsetSsbo.BindBufferBase();
   mGridInfoUbo.BindBufferBase();
   mIndexListSsbo.BindBufferBase();
   mParticleSsbo.BindBufferBase();
   glProgramUniform1i(mComputeShader.mShader, Ugrid3D::mUniformLoc.mMode, mUniforms.INSERT_PARTICLES);
   glDispatchCompute(mParticleWorkgroups.x, 1, 1);
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); 

   //mIndexListSsbo.DebugReadInt();  
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////


ParticleSystem::ParticleSystem(int n)
{
   mUniforms.mNumParticles = n;
}

void ParticleSystem::Init()
{
   mComputeShader.Init();
   mComputeShader.SetSize(glm::ivec3(mUniforms.mNumParticles, 1, 1));
   glProgramUniform1i(mComputeShader.mShader, mUniformLoc.mNumParticles, mUniforms.mNumParticles);
   
  
   if(mShader != -1)
   {
      glDeleteProgram(mShader);
   }
   mShader = InitShader(mVertexShaderName.c_str(), mFragmentShaderName.c_str());


   std::vector<glm::vec4> zero(2*mUniforms.mNumParticles, glm::vec4(0.0f));
   GLuint flags = 0;

   mParticleSsbo.Init(sizeof(Particle)*mUniforms.mNumParticles, zero.data(), flags); 

   if(mVao != -1)
   {
      glDeleteVertexArrays(1, &mVao);
   }
   glGenVertexArrays(1, &mVao);

   //Enable gl_PointCoord in shader
   glEnable(GL_POINT_SPRITE);
   //Allow setting point size in fragment shader
   glEnable(GL_PROGRAM_POINT_SIZE);
}

void ParticleSystem::Draw()
{
   glUseProgram(mShader);
   //Enable alpha blending
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE); //additive alpha blending
   //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //semitransparent alpha blending
   glDepthMask(GL_FALSE);

   mParticleSsbo.BindBufferBase();
   glBindVertexArray(mVao);
   glDrawArrays(GL_POINTS, 0, mUniforms.mNumParticles);
   
   glEnable(GL_BLEND);
   glDepthMask(GL_TRUE);
   
}

void ParticleSystem::DrawGui()
{

}

void ParticleSystem::Animate(float t, float dt)
{
   mUniforms.mTime = t;

   glUseProgram(mComputeShader.mShader);
   glProgramUniform1f(mComputeShader.mShader, mUniformLoc.mTime, mUniforms.mTime);
   mParticleSsbo.BindBufferBase();
   mComputeShader.Dispatch();  
}



ParticleCollision::ParticleCollision(int n):mParticleSystem(n), mParticleGrid(glm::ivec4(16, 16, 16, 0), aabb3D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mParticleSystem.mComputeShader.mFilename = mComputeShaderName;
}

void ParticleCollision::Init()
{
   mParticleSystem.Init();
   mParticleGrid.Init(mParticleSystem.mParticleSsbo.mBuffer, mParticleSystem.mUniforms.mNumParticles);
}

void ParticleCollision::Draw()
{
   mParticleSystem.Draw();
}

void ParticleCollision::DrawGui()
{
   mParticleSystem.DrawGui();
}

void ParticleCollision::Animate(float t, float dt)
{
   if(pGrid == nullptr)
   {
      return;
   }

   mParticleSystem.mUniforms.mTime = t;
   glUseProgram(mParticleSystem.mComputeShader.mShader);
   glProgramUniform1f(mParticleSystem.mComputeShader.mShader, mParticleSystem.mUniformLoc.mTime, mParticleSystem.mUniforms.mTime);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mIndexList, pGrid->mSsbo.mIndexList);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mObj, pGrid->mSsbo.mObj);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridCounter, pGrid->mSsbo.mGridCounter);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mGridOffset, pGrid->mSsbo.mGridOffset);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mSsboBinding.mParticles, mParticleSystem.mParticleSsbo.mBuffer);
   glBindBufferBase(GL_UNIFORM_BUFFER, pGrid->mGridInfoBinding, pGrid->mGridInfoUbo);
   
   
   glProgramUniform1i(mParticleSystem.mComputeShader.mShader, mUniformLoc.mMode, mUniforms.PREANIMATE);
   mParticleSystem.mComputeShader.Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


   glProgramUniform1i(mParticleSystem.mComputeShader.mShader, mUniformLoc.mMode, mUniforms.COLLISION_QUERY);
   mParticleSystem.mComputeShader.Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   glProgramUniform1i(mParticleSystem.mComputeShader.mShader, mUniformLoc.mMode, mUniforms.POSTANIMATE);
   mParticleSystem.mComputeShader.Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   mParticleGrid.BuildGrid();

}
