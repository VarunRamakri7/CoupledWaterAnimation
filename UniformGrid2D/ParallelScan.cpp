#include "ParallelScan.h"
#include "InitShader.h"
#include <vector>

const bool EnableTest = false;

ParallelScan::ParallelScan()
{
   
}

void ParallelScan::Init(GLuint input, int n)
{
   //n must be a power of 2
   float log2n = glm::log2(float(n));
   assert(glm::ceil(log2n)==glm::floor(log2n));


   //Create compute shader
   mComputeShader = InitShader(mComputeShaderName.c_str());

   mNumElements = n;
   mInputSsbo = input;

   if(mOutputSsbo != -1)
   {
      glDeleteBuffers(1, &mOutputSsbo);
   }

   GLuint flags = 0;
   if(EnableTest==true)
   {
      flags = GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;
   }
   
   glGenBuffers(1, &mOutputSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mOutputSsbo);
   glNamedBufferStorage(mOutputSsbo, sizeof(int) * mNumElements, nullptr, flags);

   glProgramUniform1i(mComputeShader, mNLoc, mNumElements);
}

void ParallelScan::Compute()
{
   glUseProgram(mComputeShader);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mInputBinding, mInputSsbo);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mOutputBinding, mOutputSsbo);

   //copy input to output since scan is computed in-place
   int offset = 0;
   glCopyNamedBufferSubData(mInputSsbo, mOutputSsbo, offset, offset, mNumElements*sizeof(int));
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   

   //upsweep is a reduction
   int n = mNumElements/2;
   int pass = 0;
   glProgramUniform1i(mComputeShader, mPhaseLoc, UPSWEEP_PHASE);
   for(;;)
   {
      int stride = 2<<pass;
      glProgramUniform1i(mComputeShader, mStrideLoc, stride); //STRIDE = 2, 4, 8, ...
      int workgroups = glm::ceil(n/float(mMaxWorkGroup));
      glDispatchCompute(workgroups, 1, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      if(n == 1) break;
      n = n/2;
      pass = pass+1;
      //TestReadback();
   }

   //TestReadback();

   //downsweep
   glProgramUniform1i(mComputeShader, mPhaseLoc, DOWNSWEEP_PHASE);
   for(;;)
   {
      int stride = 2<<pass;
      glProgramUniform1i(mComputeShader, mStrideLoc, stride); //STRIDE = ...
      int workgroups = glm::ceil(n/float(mMaxWorkGroup));
      glDispatchCompute(workgroups, 1, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      if(n == mNumElements/2) break;
      n = n*2;
      pass = pass-1;

      //TestReadback();
   }

   TestReadback();

}

void ParallelScan::TestReadback()
{
   if(EnableTest)
   {
      int *ptr;
      ptr = (int*) glMapNamedBuffer(mOutputSsbo, GL_READ_ONLY);
      std::vector<int> test;
      for (int i = 0; i < mNumElements; i++)
      {
         test.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
}


void ParallelScanTest()
{
   ParallelScan scan;
   
   std::vector<int> x = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

   GLuint flags = GL_DYNAMIC_STORAGE_BIT;
   GLuint ssbo = -1;
   glGenBuffers(1, &ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
   glNamedBufferStorage(ssbo, sizeof(int)*x.size(), x.data(), flags);
  
   scan.Init(ssbo, x.size());
   scan.Compute();
}