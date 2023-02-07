#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class ComputeShader //TODO inherit from Shader
{
   public:
      ComputeShader(const std::string& filename="");
      void Init();
      void UseProgram();
      void Dispatch();

      GLuint GetShader()      {return mShader;}
      std::string GetName()   {return mFilename;}

      glm::ivec3 GetWorkGroupSize() { return mWorkGroupSize; }

      void SetGridSize(glm::ivec3 grid_size);
      glm::ivec3 GetGridSize() {return mGridSize;}
      
      int GetMode()  {return mMode;}
      void SetMode(int mode);

      int GetUniformLocation(const char* name);

   private:
      void SetWorkGroupSize(glm::ivec3 size);
      std::string mFilename;
      GLuint mShader = -1;
      glm::ivec3 mWorkGroupSize = glm::ivec3(0, 0, 0);
      glm::ivec3 mNumWorkgroups = glm::ivec3(0);
      glm::ivec3 mGridSize = glm::ivec3(0);
      
      GLuint mModeLoc = -1;
      int mMode = 0;
};
