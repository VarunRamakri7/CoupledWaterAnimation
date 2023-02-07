#pragma once

#include "Module.h"
#include "ComputeShader.h"
#include "Buffer.h"

struct StencilBuffer : public Module
{
   void Init() override;
   void Reinit() override;
   void Compute() override;
   void DrawGui() override;

   Buffer& GetReadBuffer() { return mBuffer[mReadIndex]; }
   Buffer& GetWriteBuffer() { return mBuffer[mWriteIndex]; }
   void PingPong();
   void SetSubsteps(int s) { mSubsteps = s; }

   void SetShader(ComputeShader& cs);
   ComputeShader* GetpShader() { return pShader; }

   const int MODE_INIT = 0;
   const int MODE_EVOLVE = 1;
   //TODO to support multiple inits and multiple iterate passes
   /*
      int mModeInitFirst = 0;
      int mModeInitLast = 0;
      int mModeIterateFirst = 1;
      int mModeIterateLast = 1;
   */

   const int NUM_ELEMENTS_LOC = 1;

   //TODO make protected
   int mNumElements = 0;
   int mElementSize = 0;

   protected:
      ComputeShader* pShader = nullptr;
      bool mEvolve = true;
      int mReadIndex = 0;
      int mWriteIndex = 1;
      bool mEnableDebug = false;
      int mSubsteps = 1;

      //int mNumElements = 0;
      //int mElementSize = 0;
      Buffer mBuffer[2] = { Buffer(GL_SHADER_STORAGE_BUFFER), Buffer(GL_SHADER_STORAGE_BUFFER) };
};

class SphMuller : public StencilBuffer
{
   public:
      void Compute() override;
      void DrawGui() override;
   protected:
      int mModeIterateFirst = 1;
      int mModeIterateLast = 2;

};

#include "UniformGridGpu2D.h"

class SphUgrid : public StencilBuffer
{
public:
   SphUgrid();
   UniformGridSph2D mGrid;

   void Init() override;
   void Compute() override;
   void DrawGui() override;
   void ComputeFunc(int mode);

protected:
   int mModeIterateFirst = 1;
   int mModeIterateLast = 2;

};