#include "ComputeShader.h"
#include "InitShader.h"


ComputeShader::ComputeShader(const std::string& filename) :mFilename(filename)
{

}

void ComputeShader::Init()
{
   GLuint new_shader = InitShader(mFilename.c_str());

   if (mShader != -1)
   {
      glDeleteProgram(mShader);
   }
   if (new_shader != -1)
   {
      mShader = new_shader;
   }
   mModeLoc = glGetUniformLocation(mShader, "uMode");
   SetMode(mMode);

   glm::ivec3 size;
   glGetProgramiv(mShader, GL_COMPUTE_WORK_GROUP_SIZE, &size.x);
   SetWorkGroupSize(size);
}

int ComputeShader::GetUniformLocation(const char* name)
{
   return glGetUniformLocation(mShader, name);
}

void ComputeShader::SetGridSize(glm::ivec3 grid_size)
{
   mGridSize = grid_size;
   mNumWorkgroups = glm::ceil(glm::vec3(mGridSize) / glm::vec3(mWorkGroupSize));
   mNumWorkgroups = glm::max(mNumWorkgroups, glm::ivec3(1));
}

void ComputeShader::SetWorkGroupSize(glm::ivec3 size)
{ 
   mWorkGroupSize = size; 
   mNumWorkgroups = glm::ceil(glm::vec3(mGridSize) / glm::vec3(mWorkGroupSize));
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
