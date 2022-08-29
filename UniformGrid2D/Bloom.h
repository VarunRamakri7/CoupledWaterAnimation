#pragma once
#include <windows.h>
#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>

struct Bloom0
{
   Bloom0();
   void Init(glm::ivec2 output_size);
   void ComputeBloom();
   void SetInputTexture(GLuint input_tex);

   std::string vertex_shader = std::string("bloom0_vs.glsl");
   std::string fragment_shader = std::string("bloom0_fs.glsl");
   GLuint mShader;

   GLuint mAttriblessVao = -1;
   
   GLuint mFbo = -1;
   GLuint mBrightTex = -1;
   GLuint mBlurTex[2] = {GLuint(-1), GLuint(-1)};
   GLuint mInputTex = -1;
   GLuint mOutputTex = -1;
   glm::ivec2 mOutputSize;

};


struct Bloom1
{
   void Init(glm::ivec2 output_size);
   void ComputeBloom();
   void SetInputTexture(GLuint input_tex);

   std::string vertex_shader = std::string("bloom1_vs.glsl");
   std::string fragment_shader = std::string("bloom1_fs.glsl");
   GLuint mShader;

   GLuint mAttriblessVao = -1;
   
   static const int sLevels = 8;
   GLuint mBloomFbo[sLevels] = {GLuint(-1)};
   
   GLuint mDownsampleTex = -1;
   GLuint mUpsampleTex = -1;
   GLuint mInputTex = -1;

   GLuint mOutputTex = -1;
   GLuint mOutputFbo = -1;
   
   glm::ivec2 mOutputSize;

};


