#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader
{
   public:
      Shader(const std::string& vs_name, const std::string& fs_name);
      bool Init();
      void UseProgram();
      int GetUniformLocation(const char* name);
      GLuint GetShader() { return mShader; }

   protected:
      std::string mFilenames[5];
      GLuint mShader = -1;

      const int VS_INDEX = 0;
      const int TC_INDEX = 1;
      const int TE_INDEX = 2;
      const int GS_INDEX = 3;
      const int FS_INDEX = 4;
};
