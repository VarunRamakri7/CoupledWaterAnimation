#include "ComputeShader.h"
#include "InitShader.h"


ComputeShader::ComputeShader(const std::string& filename) :mFilename(filename)
{

}

void ComputeShader::Init()
{
   if (mShader != -1)
   {
      glDeleteProgram(mShader);
   }
   mShader = InitShader(mFilename.c_str());
   mModeLoc = glGetUniformLocation(mShader, "uMode");
   SetMode(mMode);
}

int ComputeShader::GetUniformLocation(const char* name)
{
   return glGetUniformLocation(mShader, name);
}

void ComputeShader::SetGridSize(glm::ivec3 grid_size)
{
   mGridSize = grid_size;
   mNumWorkgroups = glm::ceil(glm::vec3(mGridSize) / glm::vec3(mMaxWorkGroupSize));
   mNumWorkgroups = glm::max(mNumWorkgroups, glm::ivec3(1));
}

void ComputeShader::SetMaxWorkGroupSize(glm::ivec3 max_size)
{ 
   mMaxWorkGroupSize = max_size; 
   mNumWorkgroups = glm::ceil(glm::vec3(mGridSize) / glm::vec3(mMaxWorkGroupSize));
   mNumWorkgroups = glm::max(mNumWorkgroups, glm::ivec3(1));
}

void ComputeShader::UseProgram()
{
   glUseProgram(mShader);
}

void ComputeShader::Dispatch()
{
   glDispatchCompute(mNumWorkgroups.x, mNumWorkgroups.y, mNumWorkgroups.z);
}

void ComputeShader::SetMode(int mode)
{
   mMode = mode;
   if (mModeLoc >= 0 && mShader >= 0)
   {
      glProgramUniform1i(mShader, mModeLoc, mMode);
   }
}
