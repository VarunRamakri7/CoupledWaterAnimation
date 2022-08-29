#pragma once
#include <windows.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct BlitFbo
{
   void Init(glm::ivec2 input_size, glm::ivec2 output_size);
   void SetInputTexture(GLuint input_tex, int level);
   void Blit();

   glm::ivec2 mInputSize;
   glm::ivec2 mOutputSize;
   GLuint mInputTexture = -1;
   GLuint mFbo = -1;
   GLenum mFilter = GL_NEAREST;
};


struct RenderFbo
{
   void Init(glm::ivec2 output_size);
   void PreRender();
   void PostRender();

   glm::ivec2 mOutputSize;
   GLuint mOutputTexture = -1;
   GLuint mFbo = -1;
   GLuint mRbo = -1;

   //For blit
   void Blit();
   GLenum mFilter = GL_NEAREST;
};
