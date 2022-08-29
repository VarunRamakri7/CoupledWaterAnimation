#include "Bloom.h"
#include "AttriblessRendering.h"
#include "InitShader.h"


namespace BloomUniformLocs
{
   int pass = 0;
   int read_level = 1;
};


Bloom0::Bloom0()
{

}

void Bloom0::Init(glm::ivec2 output_size)
{  
   mOutputSize = output_size;

   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   glGenVertexArrays(1, &mAttriblessVao);    

   glGenTextures(1, &mBrightTex);
   glBindTexture(GL_TEXTURE_2D, mBrightTex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mOutputSize.x, mOutputSize.y, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenTextures(2, mBlurTex);
   for(int i=0; i<2; i++)
   {
      glBindTexture(GL_TEXTURE_2D, mBlurTex[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mOutputSize.x, mOutputSize.y, 0, GL_RGBA, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, 0);
   } 

   glGenTextures(1, &mOutputTex);
   glBindTexture(GL_TEXTURE_2D, mOutputTex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mOutputSize.x, mOutputSize.y, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   //Create the framebuffer object
   glGenFramebuffers(1, &mFbo);
   glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBrightTex, 0);
   for(int i=0; i<2; i++)
   {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1+i, GL_TEXTURE_2D, mBlurTex[i], 0);
   }

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mOutputTex, 0);

   //unbind the fbo
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Bloom0::SetInputTexture(GLuint input_tex)
{
   mInputTex = input_tex;
   glTextureParameteri(input_tex, GL_TEXTURE_MAX_LEVEL, 4);
   glTextureParameteri(input_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTextureParameteri(input_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTextureParameteri(input_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glGenerateTextureMipmap(input_tex);
}

void Bloom0::ComputeBloom()
{
   glUseProgram(mShader);
   glBindFramebuffer(GL_FRAMEBUFFER, mFbo); // Render to FBO.
   glViewport(0, 0, mOutputSize.x, mOutputSize.y);
   glBindVertexArray(mAttriblessVao);
   
   glUniform1i(BloomUniformLocs::pass, 0); //bright pass
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glBindTextureUnit(0, mInputTex); //read from
   draw_attribless_quad();

   for(int i=0; i<1; i++)
   {
      GLuint read_tex = mBrightTex;
      if(i > 0)
      {
         read_tex = mBlurTex[1];
      }
      glUniform1i(BloomUniformLocs::pass, 1); //horizontal blur pass
      glDrawBuffer(GL_COLOR_ATTACHMENT1);
      glBindTextureUnit(1, read_tex);
      draw_attribless_quad();

      glUniform1i(BloomUniformLocs::pass, 2); //vertical blur pass
      glDrawBuffer(GL_COLOR_ATTACHMENT2);
      glBindTextureUnit(2, mBlurTex[0]);
      draw_attribless_quad();
   }
   glBindTextureUnit(1, mBrightTex);

   glUniform1i(BloomUniformLocs::pass, 3); //bloom pass
   glDrawBuffer(GL_COLOR_ATTACHMENT3);
   glBindTextureUnit(3, mBlurTex[1]);
   draw_attribless_quad();
}

////////////////////////////////////////////////////////////////////////////////////////



void Bloom1::Init(glm::ivec2 output_size)
{  
   mOutputSize = output_size;

   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   glGenVertexArrays(1, &mAttriblessVao);    

   glCreateTextures(GL_TEXTURE_2D, 1, &mDownsampleTex);
   glTextureStorage2D(mDownsampleTex, sLevels, GL_RGBA32F, mOutputSize.x, mOutputSize.y);
   glTextureParameteri(mDownsampleTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mDownsampleTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mDownsampleTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTextureParameteri(mDownsampleTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glCreateTextures(GL_TEXTURE_2D, 1, &mUpsampleTex);
   glTextureStorage2D(mUpsampleTex, sLevels, GL_RGBA32F, mOutputSize.x, mOutputSize.y);
   glTextureParameteri(mUpsampleTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mUpsampleTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mUpsampleTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTextureParameteri(mUpsampleTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   //Create the framebuffer objects
   glCreateFramebuffers(sLevels, mBloomFbo);
   
   for(int i=0; i<sLevels; i++)
   {
      glNamedFramebufferTexture(mBloomFbo[i], GL_COLOR_ATTACHMENT0, mDownsampleTex, i);
      glNamedFramebufferTexture(mBloomFbo[i], GL_COLOR_ATTACHMENT1, mUpsampleTex, i);
   }

   glCreateTextures(GL_TEXTURE_2D, 1, &mOutputTex);
   glTextureStorage2D(mOutputTex, 1, GL_RGBA32F, mOutputSize.x, mOutputSize.y);
   glTextureParameteri(mOutputTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mOutputTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTextureParameteri(mOutputTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTextureParameteri(mOutputTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glCreateFramebuffers(1, &mOutputFbo);
   glNamedFramebufferTexture(mOutputFbo, GL_COLOR_ATTACHMENT0, mOutputTex, 0);
}

void Bloom1::SetInputTexture(GLuint input_tex)
{
   mInputTex = input_tex;
}

void Bloom1::ComputeBloom()
{
   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);
   glDisable(GL_BLEND);  
   
   glm::ivec2 pass_size = mOutputSize;

   //pass -1: brightpass
   glUniform1i(BloomUniformLocs::pass, -1);
   glBindFramebuffer(GL_FRAMEBUFFER, mBloomFbo[0]); // Render to FBO Downsample
   glBindTextureUnit(2, mInputTex);
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glViewport(0, 0, mOutputSize.x, mOutputSize.y);
   draw_attribless_quad();
   
   //pass 0: downsample
   glUniform1i(BloomUniformLocs::pass, 0);
   glBindTextureUnit(0, mDownsampleTex);
   const int max_level = sLevels-1;
   for(int i=1; i<=max_level; i++)  //i is the output mipmap level
   {
      glUniform1i(BloomUniformLocs::read_level, i-1); //downsample read_level# = input mipmap level
      pass_size = pass_size/2;

      glBindFramebuffer(GL_FRAMEBUFFER, mBloomFbo[i]); // Render to FBO Downsample
      glDrawBuffer(GL_COLOR_ATTACHMENT0);

      glViewport(0, 0, pass_size.x, pass_size.y);
      draw_attribless_quad();
   }
   
   //Pass 1: upsample
   glUniform1i(BloomUniformLocs::pass, 1);
   glBindTextureUnit(1, mUpsampleTex);
   //glBindTextureUnit(0, mDownsampleTex);
   for(int i=max_level; i>=0; i--)  //i is the output mipmap level
   {
      glUniform1i(BloomUniformLocs::read_level, i); //downsample read_level# = input mipmap level

      glBindFramebuffer(GL_FRAMEBUFFER, mBloomFbo[i]); // Render to FBO Upsample
      glDrawBuffer(GL_COLOR_ATTACHMENT1);

      glViewport(0, 0, pass_size.x, pass_size.y);
      draw_attribless_quad();
      pass_size = pass_size*2;
   }

   //pass 2: sum, tonemap, etc
   glUniform1i(BloomUniformLocs::pass, 2);
   glBindFramebuffer(GL_FRAMEBUFFER, mOutputFbo); // Render to FBO Downsample
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glViewport(0, 0, mOutputSize.x, mOutputSize.y);
   draw_attribless_quad();
}