#include "StencilImage2D.h"
#include "imgui.h"

void StencilImage2D::SetShader(ComputeShader& cs)
{
   pShader = &cs;
   pShader->SetGridSize(glm::ivec3(mGridSize, 1));
}

void StencilImage2D::Init()
{
   if(pShader != nullptr)
   {
      pShader->SetGridSize(glm::ivec3(mGridSize, 1));
      pShader->SetMaxWorkGroupSize(glm::ivec3(32, 32, 1));
      pShader->Init();
   }

   for (int i = 0; i < 2; i++)
   {
      mImage[i].SetInternalFormat(GL_RGBA32F);
      mImage[i].SetLevels(1);
      mImage[i].SetUnit(i);
      mImage[i].SetSize(glm::ivec3(mGridSize, 1));
      mImage[i].Init();
   }

   Reinit();
}

void StencilImage2D::PingPong()
{
   std::swap(mReadIndex, mWriteIndex);
   SwapUnits(mImage[mReadIndex], mImage[mWriteIndex]);
}

void StencilImage2D::Reinit()
{
   if (pShader == nullptr) return;

   //Initialize image in CS
   pShader->SetMode(MODE_INIT);
   mImage[mReadIndex].BindImageTexture(GL_READ_ONLY);
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void StencilImage2D::ReinitFromTexture(GLuint init_texture)
{
   if (pShader == nullptr) return;

   glBindTextureUnit(INIT_TEXTURE_UNIT, init_texture);

   //Initialize image in CS
   pShader->SetMode(MODE_INIT_FROM_TEXTURE);
   mImage[mReadIndex].BindImageTexture(GL_READ_ONLY);
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void StencilImage2D::Compute()
{
   if(mEvolve == false) return;
   if (pShader == nullptr) return;

   pShader->SetMode(MODE_EVOLVE);
   mImage[mReadIndex].BindImageTexture(GL_READ_ONLY);
   mImage[mWriteIndex].BindImageTexture(GL_WRITE_ONLY);
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

   PingPong();
}

void StencilImage2D::DrawGui()
{
   ImGui::Begin("CA2D");
   if (ImGui::Button("Reinit CA"))
   {
      Reinit();
   }
   ImGui::Checkbox("Evolve CA", &mEvolve);

/*
   static glm::ivec3 new_size = mImage[mReadIndex].GetSize();
   ImGui::SliderInt2("Size", &new_size.x, 0, 1024);
   ImGui::SameLine();
   if (ImGui::Button("Apply"))
   {
      mImage[mReadIndex].Resize(new_size);
      mImage[mWriteIndex].Resize(new_size);
      new_size = mImage[mReadIndex].GetSize();
   }
*/
   static glm::ivec3 new_size = glm::ivec3(mGridSize, 1);
   ImGui::SliderInt2("Size", &new_size.x, 0, 1024);
   ImGui::SameLine();
   if (ImGui::Button("Apply"))
   {
      mImage[mReadIndex].Resize(new_size);
      mImage[mWriteIndex].Resize(new_size);
      pShader->SetGridSize(new_size);
      mGridSize = glm::ivec2(new_size);
   }

   ImGui::End();
}