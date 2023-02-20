#include "ParallelScan.h"
#include "InitShader.h"
#include <iostream>


ParallelScan::ParallelScan()
{
   
}

void ParallelScan::Init(GLuint input, int n, bool readback)
{
   mEnableReadback = readback;
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
   if(mEnableReadback == true)
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
      //Readback();
   }

   //Readback();

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

      //Readback();
   }

   //Readback();

}

std::vector<int> ParallelScan::Readback()
{
   /*
   std::vector<int> readback;
   if(mEnableReadback==true)
   {
      int *ptr;
      ptr = (int*) glMapNamedBuffer(mOutputSsbo, GL_READ_ONLY);
      for (int i = 0; i < mNumElements; i++)
      {
         readback.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
   */

   std::vector<int> readback;
   if (mEnableReadback == true)
   {
      readback.resize(mNumElements);
      glGetNamedBufferSubData(mOutputSsbo, 0, sizeof(int) * mNumElements, readback.data());
   }
   
   return readback;
}


void ParallelScanTest()
{
   ParallelScan scan;
   
   //std::vector<int> x = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
   std::vector<int> x = { 1,0,1,0,1,2,1,2,1,2,0,1,0,2,1,0 };
   std::vector<int> truth(x.size());

   int sum = 0;
   for (int i = 0; i < x.size(); i++)
   {
      truth[i]=sum;
      sum += x[i];
   }
   

   GLuint flags = GL_DYNAMIC_STORAGE_BIT;
   GLuint ssbo = -1;
   glGenBuffers(1, &ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
   glNamedBufferStorage(ssbo, sizeof(int)*x.size(), x.data(), flags);
  
   scan.Init(ssbo, x.size(), true);
   scan.Compute();

   std::vector<int> result = scan.Readback();
   
   if (result != truth)
   {
      std::cout << "Failed" << std::endl;
   }
}