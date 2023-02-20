#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include "GL/glew.h"
#include <glm/glm.hpp>

/*
Note: This function is intended to be used for prototyping only.

Future work TODO:
Priority 1. Handle uniform blocks
Priority 2. Handle per variable ranges cleanly
Priority 3. Do something smart with matrix uniforms, like if named V, set with glm::lookAt parameters, if named P set with glm::perspective parameters
*/

void UniformGuiBasic(GLuint program);
void UniformGui(GLuint program);

struct UniformGuiContext
{
   using uniform_val = std::variant<glm::ivec4, glm::vec4, glm::bvec4>;

   struct UniformVar
   {
      std::string mName = "";
      GLint mLoc = -1;
      GLint mType = 0;
      GLint mArraySize = 0;
      std::vector<uniform_val> mValue;
      UniformVar* pRange = nullptr;
      std::string mFormat = "%.3f"; //for ImGui widgets
   };

   GLuint mShader = -1;
   std::vector<UniformVar> mUniforms;
   std::map<std::string, int> mNameMap;

   UniformGuiContext(GLuint program);
   void Init(GLuint program);
};

void UniformGui(UniformGuiContext& context);

