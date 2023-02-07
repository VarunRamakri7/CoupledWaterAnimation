#include "BlitFbo.h"

void BlitFbo::Init(glm::ivec2 input_size, glm::ivec2 output_size)
{
   mInputSize = input_size;
   mOutputSize = output_size;

   //Create an FBO for blitting to the screen
   glCreateFramebuffers(1, &mFbo);
}

void BlitFbo::Init(glm::ivec2 size)
{
   mInputSize = size;
   mOutputSize = size;

   //TODO delete old fbo
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





void RenderFbo::Init(glm::ivec2 output_size, int n)
{
   mOutputSize = output_size;
   mNumAttachments = n;
   mOutputTextures.resize(mNumAttachments);


   //Create an FBO 
   glCreateFramebuffers(1, &mFbo);
   
   //Create depth RBO
   glCreateRenderbuffers(1, &mRbo);
   glNamedRenderbufferStorage(mRbo, GL_DEPTH_COMPONENT, mOutputSize.x, mOutputSize.y);

   //attach depth renderbuffer to depth attachment
   glNamedFramebufferRenderbuffer(mFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRbo);

   //Create texture objects and set initial wrapping and filtering state
   const int levels = 1;
   glCreateTextures(GL_TEXTURE_2D, mNumAttachments, mOutputTextures.data());
   for(int i=0; i< mNumAttachments; i++)
   {
      glTextureStorage2D(mOutputTextures[i], levels, GL_RGBA32F, mOutputSize.x, mOutputSize.y);
      glTextureParameteri(mOutputTextures[i], GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTextureParameteri(mOutputTextures[i], GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTextureParameteri(mOutputTextures[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTextureParameteri(mOutputTextures[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      //attach the texture we just created to the FBO
      glNamedFramebufferTexture(mFbo, GL_COLOR_ATTACHMENT0+i, mOutputTextures[i], 0);
   }
}

void RenderFbo::PreRender()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mFbo); // Render to FBO.
   const GLenum buffers [] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, 
                           GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, 
                           GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11, 
                           GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};

   glDrawBuffers(mNumAttachments, buffers);
   //glDrawBuffer(GL_COLOR_ATTACHMENT0);
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