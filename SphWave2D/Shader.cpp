#include "Shader.h"
#include "InitShader.h"

Shader::Shader(const std::string& vs_name, const std::string& fs_name)
{
   mFilenames[VS_INDEX] = vs_name;
   mFilenames[FS_INDEX] = fs_name;
}

bool Shader::Init()
{
   bool success = true;
   GLuint new_shader = -1;
   if(mFilenames[TC_INDEX]=="" && mFilenames[TE_INDEX] == "" && mFilenames[GS_INDEX] == "")
   {
      new_shader = InitShader(mFilenames[VS_INDEX].c_str(), mFilenames[FS_INDEX].c_str());
   }

   if (new_shader == -1) // loading failed
   {
      success = false;
      DebugBreak();
   }
   else
   {
      if (mShader != -1)
      {
         glDeleteProgram(mShader);
      }
      mShader = new_shader;
      mModeLoc = glGetUniformLocation(mShader, "uMode");
      SetMode(mMode);
   }
   return success;
}

int Shader::GetUniformLocation(const char* name)
{
   return glGetUniformLocation(mShader, name);
}

void Shader::UseProgram()
{
   glUseProgram(mShader);
}

void Shader::SetMode(int mode)
{
   mMode = mode;
   if (mModeLoc >= 0 && mShader >= 0)
   {
      glProgramUniform1i(mShader, mModeLoc, mMode);
   }
}