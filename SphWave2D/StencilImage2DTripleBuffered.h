#pragma once

#include "Module.h"
#include "ComputeShader.h"
#include "Buffer.h"
#include "ImageTexture.h"

struct StencilImage2DTripleBuffered :public Module
{
   void Init() override;
   void Reinit() override;
   void Compute() override;
   void DrawGui() override;

   void ReinitFromTexture(GLuint init_texture);

   ImageTexture& GetWriteImage() { return mImage[mWriteIndex]; }
   ImageTexture& GetReadImage(int i) { return mImage[mReadIndex[i]]; }
   
   void PingPong();

   void SetShader(ComputeShader& cs);
   ComputeShader* GetpShader() { return pShader; }
   glm::ivec2 GetGridSize() { return mGridSize; }

   const int MODE_INIT = 0;
   const int MODE_INIT_FROM_TEXTURE = 1;
   const int MODE_EVOLVE = 2;

   const int INIT_TEXTURE_UNIT = 3;

private:
   ComputeShader* pShader = nullptr;
   bool mEvolve = true;
   int mReadIndex[2] = {0, 1};
   int mWriteIndex = 2;

   glm::ivec2 mGridSize = glm::ivec2(64, 64);
   ImageTexture mImage[3] = { ImageTexture(GL_TEXTURE_2D), ImageTexture(GL_TEXTURE_2D), ImageTexture(GL_TEXTURE_2D) };
};

