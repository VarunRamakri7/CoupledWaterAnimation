#include "Bloom.h"
#include "AttriblessRendering.h"
#include "InitShader.h"


namespace BloomUniformLocs
{
   int pass = 0;
};


Bloom::Bloom()
{

}

void Bloom::Init(int width, int height)
{  
   mWidth = width;
   mHeight = height;

   bloom_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   glGenVertexArrays(1, &attribless_vao);    

   glGenTextures(1, &bright_tex);
   glBindTexture(GL_TEXTURE_2D, bright_tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenTextures(2, blur_tex);
   for(int i=0; i<2; i++)
   {
      glBindTexture(GL_TEXTURE_2D, blur_tex[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, 0);
   } 

   glGenTextures(1, &output_tex);
   glBindTexture(GL_TEXTURE_2D, output_tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   //Create the framebuffer object
   glGenFramebuffers(1, &fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bright_tex, 0);
   for(int i=0; i<2; i++)
   {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1+i, GL_TEXTURE_2D, blur_tex[i], 0);
   }

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, output_tex, 0);

   //unbind the fbo
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Bloom::ComputeBloom(GLuint input_tex)
{
   glTextureParameteri(input_tex, GL_TEXTURE_MAX_LEVEL, 4);
   glTextureParameteri(input_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTextureParameteri(input_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTextureParameteri(input_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glGenerateTextureMipmap(input_tex);

   glUseProgram(bloom_shader);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
   glViewport(0, 0, mWidth, mHeight);
   glBindVertexArray(attribless_vao);
   
   glUniform1i(BloomUniformLocs::pass, 0); //bright pass
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glBindTextureUnit(0, input_tex); //read from
   draw_attribless_quad();

   for(int i=0; i<1; i++)
   {
      GLuint read_tex = bright_tex;
      if(i > 0)
      {
         read_tex = blur_tex[1];
      }
      glUniform1i(BloomUniformLocs::pass, 1); //horizontal blur pass
      glDrawBuffer(GL_COLOR_ATTACHMENT1);
      glBindTextureUnit(1, read_tex);
      draw_attribless_quad();

      glUniform1i(BloomUniformLocs::pass, 2); //vertical blur pass
      glDrawBuffer(GL_COLOR_ATTACHMENT2);
      glBindTextureUnit(2, blur_tex[0]);
      draw_attribless_quad();
   }
   glBindTextureUnit(1, bright_tex);

   glUniform1i(BloomUniformLocs::pass, 3); //bloom pass
   glDrawBuffer(GL_COLOR_ATTACHMENT3);
   glBindTextureUnit(3, blur_tex[1]);
   draw_attribless_quad();


}