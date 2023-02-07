#pragma once

#include "Module.h"
#include "ComputeShader.h"
#include "Buffer.h"
#include "ImageTexture.h"
#include <vector>

//TODO rename to ImageStencil

struct ImageStencil:public Module
{
   void Init();
   void Reinit() override;
   void Compute() override;
   void DrawGui() override;

   void ReinitFromTexture(GLuint init_texture);
   void ComputeFunc(int mode);

   ImageTexture& GetReadImage(int i)  {return mImage[mReadIndex[i]]; }
   
   int GetNumReadImages()  {return mNumReadImages;}
   ImageTexture& GetWriteImage() {return mImage[mWriteIndex]; }
   void PingPong();

   void SetShader(ComputeShader& cs);
   ComputeShader* GetpShader()   {return pShader;}
   void SetGridSize(glm::ivec3 grid_size) {mGridSize = glm::max(glm::ivec3(1), grid_size);}
   glm::ivec3 GetGridSize()  {return mGridSize;}

   void SetSubsteps(int s) {mSubsteps=s;}

   void SetNumBuffers(int nread);

   int mMODE_INIT_FIRST = 0;
   int mMODE_INIT_LAST = 1;
   int MODE_ITERATE_FIRST = 2;
   int MODE_ITERATE_LAST = 2;

   int mMODE_INIT_FROM_TEXTURE = -1;
   const int INIT_TEXTURE_UNIT = 2; //texture unit to use when intializing from texture

   private:
      ComputeShader* pShader = nullptr;
      bool mIterate = true;
      std::vector<int> mReadIndex = std::vector<int>(1, 0);

      //TODO separate concepts of numreadimage, numwriteimages vs pingpongbuffers (single, double, triple)

      int mWriteIndex = 0;
      int mNumImages = 1;
      int mNumReadImages = 1;
      const int mNumWriteImages = 1;
      int mSubsteps = 1;

      glm::ivec3 mGridSize = glm::ivec3(0, 0, 0);
      std::vector<ImageTexture> mImage = std::vector<ImageTexture>(1);
};
