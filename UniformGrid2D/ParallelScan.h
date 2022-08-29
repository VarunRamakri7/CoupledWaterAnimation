#pragma once

#include <windows.h>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class ParallelScan
{
   public:
      std::string mComputeShaderName = std::string("prefix_sum_cs.glsl");
      GLuint mComputeShader = -1;
      const int mMaxWorkGroup = 1024;

      GLuint mInputSsbo = -1;
      GLuint mOutputSsbo = -1;

      int mInputBinding = 0;
      int mOutputBinding = 1;

      int mNumElements;


      const int UPSWEEP_PHASE=0;
      const int DOWNSWEEP_PHASE=1;
      GLuint mPhaseLoc = 0;
      GLuint mStrideLoc = 1;
      GLuint mNLoc = 2;

      ParallelScan();
      void Init(GLuint input, int n);
      void Compute();

      void TestReadback();
};

void ParallelScanTest();
