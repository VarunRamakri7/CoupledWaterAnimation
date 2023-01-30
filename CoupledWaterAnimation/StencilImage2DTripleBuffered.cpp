#include "StencilImage2DTripleBuffered.h"
#include "../imgui-master/imgui.h"

void StencilImage2DTripleBuffered::SetShader(ComputeShader& cs)
{
   pShader = &cs;
   pShader->SetGridSize(glm::ivec3(mGridSize, 1));
}

void StencilImage2DTripleBuffered::Init()
{
   if (pShader != nullptr)
   {
      pShader->SetGridSize(glm::ivec3(mGridSize, 1));
      pShader->SetMaxWorkGroupSize(glm::ivec3(32, 32, 1));
      pShader->Init();
   }

   for (int i = 0; i < 3; i++)
   {
      mImage[i].SetInternalFormat(GL_RGBA32F);
      mImage[i].SetLevels(1);
      mImage[i].SetUnit(i);
      mImage[i].SetSize(glm::ivec3(mGridSize, 1));
      mImage[i].SetFilter(glm::ivec2(GL_LINEAR, GL_LINEAR));
      mImage[i].SetWrap(glm::ivec3(GL_CLAMP_TO_EDGE));
      mImage[i].Init();
   }

   Reinit();
}

void StencilImage2DTripleBuffered::PingPong()
{
   std::swap(mWriteIndex, mReadIndex[0]);
   std::swap(mReadIndex[0], mReadIndex[1]);

   SwapUnits(mImage[mWriteIndex], mImage[mReadIndex[0]]);
   SwapUnits(mImage[mReadIndex[0]], mImage[mReadIndex[1]]);
}

void StencilImage2DTripleBuffered::Reinit()
{
   if (pShader == nullptr) return;

   //Initialize image in CS. Iterate twice so that both read buffers get updated. 
   //Alternatively, you could have 2 output images for this pass and update in one pass
   pShader->SetMode(MODE_INIT);
   for(int i=0; i<2; i++)
   {
      mImage[mReadIndex[0]].BindImageTexture(GL_READ_ONLY);
      mImage[mReadIndex[1]].BindImageTexture(GL_READ_ONLY);
      mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
      pShader->UseProgram();
      pShader->Dispatch();
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      PingPong();
   }
}

void StencilImage2DTripleBuffered::ReinitFromTexture(GLuint init_texture)
{
   if (pShader == nullptr) return;

   glBindTextureUnit(INIT_TEXTURE_UNIT, init_texture);

   //Initialize image in CS
   pShader->SetMode(MODE_INIT_FROM_TEXTURE);
   mImage[mReadIndex[0]].BindImageTexture(GL_READ_ONLY);
   mImage[mReadIndex[1]].BindImageTexture(GL_READ_ONLY);
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void StencilImage2DTripleBuffered::Compute()
{
   if (mEvolve == false) return;
   if (pShader == nullptr) return;

   pShader->SetMode(MODE_EVOLVE);
   mImage[mReadIndex[0]].BindImageTexture(GL_READ_ONLY);
   mImage[mReadIndex[1]].BindImageTexture(GL_READ_ONLY);
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void StencilImage2DTripleBuffered::DrawGui()
{
   ImGui::Begin(pShader->GetName().c_str());
   if (ImGui::Button("Reinit"))
   {
      Reinit();
   }
   ImGui::Checkbox("Evolve", &mEvolve);

   static glm::ivec3 new_size = glm::ivec3(mGridSize, 1);
   static bool apply_enabled = false;
   if (ImGui::SliderInt2("Size", &new_size.x, 0, 1024))
   {
      apply_enabled = true;
   }
   ImGui::SameLine();
   if(apply_enabled)
   {
      if (ImGui::Button("Apply"))
      {
         mImage[mReadIndex[0]].Resize(new_size);
         mImage[mReadIndex[1]].Resize(new_size);
         mImage[mWriteIndex].Resize(new_size);
         pShader->SetGridSize(new_size);
         mGridSize = glm::ivec2(new_size);
      }
   }

   ImGui::End();
}