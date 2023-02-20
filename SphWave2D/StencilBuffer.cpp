#include "StencilBuffer.h"
#include "imgui.h"

void StencilBuffer::SetShader(ComputeShader& cs)
{
   pShader = &cs;
   pShader->SetGridSize(glm::ivec3(mNumElements, 1, 1));
}

void StencilBuffer::Init()
{
   if (pShader != nullptr)
   {
      pShader->SetGridSize(glm::ivec3(mNumElements, 1, 1));
      //pShader->SetMaxWorkGroupSize(glm::ivec3(1024, 1, 1));
      pShader->Init();

      glProgramUniform1i(pShader->GetShader(), NUM_ELEMENTS_LOC, mNumElements);
   }

   for (int i = 0; i < 2; i++)
   {
      int size = mNumElements*mElementSize;
      mBuffer[i].mBinding = i;
      mBuffer[i].mEnableDebug = mEnableDebug;
      mBuffer[i].Init(size);
   }

   Reinit();
}

void StencilBuffer::PingPong()
{
   std::swap(mReadIndex, mWriteIndex);
   SwapBindings(mBuffer[mReadIndex], mBuffer[mWriteIndex]);
}

void StencilBuffer::Reinit()
{
   if (pShader == nullptr) return;

   //Initialize image in CS
   pShader->SetMode(MODE_INIT);
   mBuffer[mReadIndex].BindBufferBase();
   mBuffer[mWriteIndex].BindBufferBase();
   pShader->UseProgram();
   pShader->Dispatch();
   glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

   PingPong();
}

void StencilBuffer::Compute()
{
   if (mEvolve == false) return;
   if (pShader == nullptr) return;

   pShader->UseProgram();
   pShader->SetMode(MODE_EVOLVE);
   for(int s=0; s<mSubsteps; s++)
   {
      mBuffer[mReadIndex].BindBufferBase();
      mBuffer[mWriteIndex].BindBufferBase();
      pShader->Dispatch();
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      PingPong();
   }

   
}

void StencilBuffer::DrawGui()
{
   ImGui::Begin(pShader->GetName().c_str());
   if (ImGui::Button("Reinit"))
   {
      Reinit();
   }
   ImGui::Checkbox("Evolve", &mEvolve);

   static int new_size = mNumElements;
   ImGui::SliderInt("Size", &new_size, 0, 1024);
   ImGui::SameLine();
   if (ImGui::Button("Apply"))
   {
      //mBuffer[mReadIndex].Resize(new_size);//TODO
      //mBuffer[mWriteIndex].Resize(new_size);//TODO
      pShader->SetGridSize(glm::ivec3(new_size, 1, 1));
      mNumElements = new_size;
   }

   ImGui::End();
}


#include "UniformGui.h"
#include "ParallelScan.h"
#include "UniformGridGpu2D.h"
#include "Timer.h"

void SphMuller::Compute()
{
   if (mEvolve == false) return;
   if (pShader == nullptr) return;

   static GpuTimer timer;
   timer.Restart("Sph Muller start");

   pShader->UseProgram();
   for(int substep = 0; substep < 1; substep++)
   {
      for(int i = mModeIterateFirst; i<= mModeIterateLast; i++)
      {  
         mBuffer[mReadIndex].BindBufferBase();
         mBuffer[mWriteIndex].BindBufferBase();
         pShader->SetMode(i);
         pShader->Dispatch();
         glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
         PingPong();
         //mBuffer[mReadIndex].DebugReadFloat();
      }
   }
   timer.Stop("Sph Muller end");
}

void SphMuller::DrawGui()
{
   StencilBuffer::DrawGui();

   static UniformGuiContext context(pShader->GetShader());
   if (pShader->GetShader() != context.mShader)
   {
      context.Init(pShader->GetShader());
   }
   UniformGui(context);
}

SphUgrid::SphUgrid():mGrid(glm::ivec2(32, 32), aabb2D(glm::vec2(0.0f), glm::vec2(2.0*4.8f)))
{
   //CellSize must be bigger than 2*kernel_radius
}

void SphUgrid::Init()
{
   mEnableDebug = false;
   StencilBuffer::Init();
   mGrid.Init(GetReadBuffer().mBuffer, 0, mNumElements);
}

void SphUgrid::Compute()
{
   static int iteration=-1;
   iteration++;

   if (mEvolve == false) return;
   if (pShader == nullptr) return;

   static GpuTimer timer;
   timer.Restart("Sph start");

   for (int substep = 0; substep < mSubsteps; substep++)
   {
      mGrid.mSsbo.mParticles = GetReadBuffer().mBuffer;
      mGrid.CollisionQuery();

      pShader->UseProgram();
      for (int i = mModeIterateFirst; i <= mModeIterateLast; i++)
      {
         mBuffer[mReadIndex].BindBufferBase();
         mBuffer[mWriteIndex].BindBufferBase();
         pShader->SetMode(i);
         pShader->Dispatch();
         glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
         PingPong();
         mBuffer[mReadIndex].DebugReadFloat();
      }
   }
   timer.Stop("Sph end");
}

void SphUgrid::DrawGui()
{
   StencilBuffer::DrawGui();

   static UniformGuiContext context(pShader->GetShader());
   if (pShader->GetShader() != context.mShader)
   {
      context.Init(pShader->GetShader());
   }
   UniformGui(context);
}



