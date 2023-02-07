#pragma once
#include <windows.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct BlitFbo
{
   void Init(glm::ivec2 size);
   void Init(glm::ivec2 input_size, glm::ivec2 output_size);
   void SetInputTexture(GLuint input_tex, int level=0);
   void Blit();

   glm::ivec2 mInputSize;
   glm::ivec2 mOutputSize;
   GLuint mInputTexture = -1;
   GLuint mFbo = -1;
   GLenum mFilter = GL_NEAREST;
};


struct RenderFbo
{
   void Init(glm::ivec2 output_size, int n=1);
   void PreRender();
   void PostRender();

   glm::ivec2 mOutputSize;
   std::vector<GLuint> mOutputTextures;
   GLuint mFbo = -1;
   GLuint mRbo = -1;
   int mNumAttachments = 0;

   //For blit
   void Blit();
   GLenum mFilter = GL_NEAREST;
};
