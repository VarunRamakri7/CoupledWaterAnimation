#include "ImageTexture.h"

ImageTexture::ImageTexture(GLenum target):mTarget(target)
{

}

void ImageTexture::Init()
{
   if (mTexture != -1)
   {
      glDeleteTextures(1, &mTexture);
   }

   glCreateTextures(mTarget, 1, &mTexture);

   if(mTarget==GL_TEXTURE_2D)
   {
      glTextureStorage2D(mTexture, mLevels, mInternalFormat, mSize.x, mSize.y);
   }
   //TO DO handle other targets

   glTextureParameteri(mTexture, GL_TEXTURE_WRAP_S, mWrap[0]);
   glTextureParameteri(mTexture, GL_TEXTURE_WRAP_T, mWrap[1]);
   glTextureParameteri(mTexture, GL_TEXTURE_MIN_FILTER, mFilter[0]);
   glTextureParameteri(mTexture, GL_TEXTURE_MAG_FILTER, mFilter[1]);
}

void ImageTexture::BindImageTexture(GLenum access)
{
   const int level = 0;
   const bool layered = false;
   const int layer = 0;
   glBindImageTexture(mUnit, mTexture, level, layered, layer, access, mInternalFormat);
}

void ImageTexture::BindImageTexture(int level, bool layered, int layer, GLenum access)
{
   glBindImageTexture(mUnit, mTexture, level, layered, layer, access, mInternalFormat);
}

void ImageTexture::BindTextureUnit()
{
   glBindTextureUnit(mUnit, mTexture);
}

/*
void ImageTexture::TextureParameter(GLenum pname, GLint param)
{
   glTextureParameteri(mTexture, pname, param);
}
*/

void ImageTexture::SetSize(glm::ivec3 size)
{
   if (mTexture != -1)
   {
      Resize(size);
      return;
   }
   mSize = size;
}

void ImageTexture::Resize(glm::ivec3 size)
{
   glm::ivec3 old_size = mSize;
   GLuint old_tex = mTexture;

   mSize = size;
   mTexture = -1;
   Init();

   if (mTarget == GL_TEXTURE_2D)
   {
      static GLuint fbo = -1;
      if (fbo == -1)
      {
         glGenFramebuffers(1, &fbo);
      }

      const int old_fbo = 0;
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, old_tex, 0);
      
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      glm::ivec3 overlap = glm::min(old_size, mSize);
      glCopyTextureSubImage2D(mTexture, 0, 0, 0, 0, 0, overlap.x, overlap.y);
      glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
      glDeleteTextures(1, &old_tex);
   }
}

#include <algorithm>

void SwapUnits(ImageTexture& i0, ImageTexture& i1)
{
   std::swap(i0.mUnit, i1.mUnit);
}