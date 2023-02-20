#include "StencilImage2D.h"
#include "UniformGui.h"
#include "imgui.h"

void ImageStencil::SetShader(ComputeShader& cs)
{
   pShader = &cs;
   pShader->SetGridSize(mGridSize);
}

void ImageStencil::Init()
{
   if(pShader != nullptr)
   {
      pShader->SetGridSize(mGridSize);
      pShader->Init();
   }

   GLint texture_target = GL_TEXTURE_3D;
   if (mGridSize.z == 1)
   {
      texture_target = GL_TEXTURE_2D;
      if (mGridSize.y == 1)
      {
         texture_target = GL_TEXTURE_1D;
      }
   }

   for (int i = 0; i < mNumImages; i++)
   {
      mImage[i].SetTarget(texture_target);
      mImage[i].SetInternalFormat(GL_RGBA32F);
      mImage[i].SetLevels(1);
      mImage[i].SetUnit(i);
      mImage[i].SetSize(mGridSize);
      mImage[i].Init();
   }

   Reinit();
}

void ImageStencil::SetNumBuffers(int n)
{ 
   mNumImages = glm::max(1, n);
   mImage.resize(mNumImages);
  
   if (mNumImages == 1)
   {
      mNumReadImages = 1;
      mReadIndex.resize(mNumReadImages);
      mReadIndex[0] = 0;
      mWriteIndex = 0;
   }
   else
   {
      mNumReadImages = mNumImages -1;
      mReadIndex.resize(mNumReadImages);
      for (int i = 0; i < mNumReadImages; i++)
      {
         mReadIndex[i] = i;
      }
      mWriteIndex = mNumImages - 1;
   }
   
}

void ImageStencil::PingPong()
{
   //Swap all buffers
   if(mNumImages == 1) return;

   std::swap(mWriteIndex, mReadIndex[0]);
   for(int i=0; i<mNumReadImages-1; i++)
   {
      std::swap(mReadIndex[i], mReadIndex[i+1]);
   }

   SwapUnits(mImage[mWriteIndex], mImage[mReadIndex[0]]);
   for (int i = 0; i < mNumReadImages-1; i++)
   {
      SwapUnits(mImage[mReadIndex[i]], mImage[mReadIndex[i+1]]);
   }
}

void ImageStencil::Reinit()
{
   if (pShader == nullptr) return;

   //Initialize image in CS. Iterate twice so that both read buffers get updated. 
   pShader->UseProgram();
   for (int i = 0; i < mNumReadImages; i++)
   {
      pShader->SetMode(mMODE_INIT_FIRST+i);
      for(int j=0; j<mNumReadImages; j++)
      {
         mImage[mReadIndex[j]].BindImageTexture(GL_READ_ONLY);
      }
      mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
      
      pShader->Dispatch();
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      PingPong();
   }
}

void ImageStencil::ComputeFunc(int mode)
{
   pShader->UseProgram(); 
   pShader->SetMode(mode);
   for (int j = 0; j < mNumReadImages; j++)
   {
      mImage[mReadIndex[j]].BindImageTexture(GL_READ_ONLY);
   }
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);

   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
   PingPong();
}

void ImageStencil::ReinitFromTexture(GLuint init_texture)
{
   if (pShader == nullptr) return;

   glBindTextureUnit(INIT_TEXTURE_UNIT, init_texture);

   //Initialize image in CS
   pShader->SetMode(mMODE_INIT_FROM_TEXTURE);
   for (int j = 0; j < mNumReadImages; j++)
   {
      mImage[mReadIndex[j]].BindImageTexture(GL_READ_ONLY);
   }
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void ImageStencil::Compute()
{
   if(mIterate == false) return;
   if (pShader == nullptr) return;

   pShader->UseProgram();
   for(int i=0; i<mSubsteps; i++)
   {
      for(int m = MODE_ITERATE_FIRST; m<= MODE_ITERATE_LAST; m++)
      {
         pShader->SetMode(m);
         for (int j = 0; j < mNumReadImages; j++)
         {
            mImage[mReadIndex[j]].BindImageTexture(GL_READ_ONLY);
         }
         mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
      
         pShader->Dispatch();
         glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
         PingPong();
      }
   }
   
}

void ImageStencil::DrawGui()
{
   ImGui::Begin(pShader->GetName().c_str());
   if (ImGui::Button("ReInit"))
   {
      Reinit();
   }
   ImGui::Checkbox("Iterate", &mIterate);

   static glm::ivec3 new_size = mGridSize;
   ImGui::SliderInt2("Size", &new_size.x, 0, 1024);
   ImGui::SameLine();
   if (ImGui::Button("Apply"))
   {
      for (int j = 0; j < mNumReadImages; j++)
      {
         mImage[mReadIndex[j]].Resize(new_size);
      }
      mImage[mWriteIndex].Resize(new_size);
      pShader->SetGridSize(new_size);
      mGridSize = new_size;
   }

   ImGui::End();

   static UniformGuiContext context(pShader->GetShader());
   if (pShader->GetShader() != context.mShader)
   {
      context.Init(pShader->GetShader());
   }
   UniformGui(context);
}