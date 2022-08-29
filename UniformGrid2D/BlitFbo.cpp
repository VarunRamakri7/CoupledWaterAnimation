#include "BlitFbo.h"

void BlitFbo::Init(glm::ivec2 input_size, glm::ivec2 output_size)
{
   mInputSize = input_size;
   mOutputSize = output_size;

   //Create an FBO for blitting to the screen
   glCreateFramebuffers(1, &mFbo);
}

void BlitFbo::SetInputTexture(GLuint input_tex, int level)
{
   mInputTexture = input_tex;
   glNamedFramebufferTexture(mFbo, GL_COLOR_ATTACHMENT0, mInputTexture, level);
}

void BlitFbo::Blit()
{
   //blit fbo texture to screen
   const GLuint default_framebuffer = 0;
   glNamedFramebufferReadBuffer(mFbo, GL_COLOR_ATTACHMENT0);
   glBlitNamedFramebuffer(mFbo, default_framebuffer, 0, 0, mInputSize.x, mInputSize.y, 0, 0, mOutputSize.x, mOutputSize.y, GL_COLOR_BUFFER_BIT, mFilter);
}





void RenderFbo::Init(glm::ivec2 output_size)
{
   mOutputSize = output_size;

   //Create a texture object and set initial wrapping and filtering state
   const int levels = 1;
   glCreateTextures(GL_TEXTURE_2D, 1, &mOutputTexture);
   glTextureStorage2D(mOutputTexture, levels, GL_RGBA32F, mOutputSize.x, mOutputSize.y);
   glTextureParameteri(mOutputTexture, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTextureParameteri(mOutputTexture, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTextureParameteri(mOutputTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTextureParameteri(mOutputTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   //Create depth RBO
   glCreateRenderbuffers(1, &mRbo);
   glNamedRenderbufferStorage(mRbo, GL_DEPTH_COMPONENT, mOutputSize.x, mOutputSize.y);

   //Create an FBO for blitting to the screen
   glCreateFramebuffers(1, &mFbo);
   //attach the texture we just created to color attachment 1
   glNamedFramebufferTexture(mFbo, GL_COLOR_ATTACHMENT0, mOutputTexture, 0);
   //attach depth renderbuffer to depth attachment
   glNamedFramebufferRenderbuffer(mFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRbo); 
}

void RenderFbo::PreRender()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mFbo); // Render to FBO.
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glViewport(0, 0, mOutputSize.x, mOutputSize.y);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderFbo::PostRender()
{
   //resume drawing to the back buffer
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glDrawBuffer(GL_BACK);
}

void RenderFbo::Blit()
{
   //blit fbo texture to screen
   const GLuint default_framebuffer = 0;
   glNamedFramebufferReadBuffer(mFbo, GL_COLOR_ATTACHMENT0);
   glBlitNamedFramebuffer(mFbo, default_framebuffer, 0, 0, mOutputSize.x, mOutputSize.y, 0, 0, mOutputSize.x, mOutputSize.y, GL_COLOR_BUFFER_BIT, mFilter);
}