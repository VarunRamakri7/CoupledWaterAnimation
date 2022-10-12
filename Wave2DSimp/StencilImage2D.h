#pragma once

#include "Module.h"
#include "ComputeShader.h"
#include "Buffer.h"
#include "ImageTexture.h"

struct StencilImage2D :public Module
{
   void Init() override;
   void Reinit() override;
   void Compute() override;
   void DrawGui() override;

   void ReinitFromTexture(GLuint init_texture);

   ImageTexture& GetReadImage()  {return mImage[mReadIndex]; }
   ImageTexture& GetWriteImage() {return mImage[mWriteIndex]; }
   void PingPong();

   void SetShader(ComputeShader& cs);
   ComputeShader* GetpShader()   {return pShader;}
   glm::ivec2 GetGridSize()  {return mGridSize;}

   const int MODE_INIT = 0;
   const int MODE_INIT_FROM_TEXTURE = 1;
   const int MODE_EVOLVE = 2;
   const int INIT_TEXTURE_UNIT = 2;

   private:
      ComputeShader* pShader = nullptr;
      bool mEvolve = true;
      int mReadIndex = 0;
      int mWriteIndex = 1;

      glm::ivec2 mGridSize = glm::ivec2(64, 64);
      ImageTexture mImage[2] = { ImageTexture(GL_TEXTURE_2D), ImageTexture(GL_TEXTURE_2D) };
};
