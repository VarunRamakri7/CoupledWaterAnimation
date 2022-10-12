#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

class ImageTexture
{
   public:
      
      ImageTexture(GLenum target);
      void Init();
      void BindImageTexture(GLenum access);
      void BindImageTexture(int level, bool layered, int layer, GLenum access);
   
      void BindTextureUnit();
      //void TextureParameter(GLenum pname, GLint param);

      void SetInternalFormat(GLint iformat) {mInternalFormat = iformat;}
      void SetLevels(GLint levels) {mLevels = levels;}

      void SetFilter(glm::ivec2 filter) {mFilter = filter;}
      void SetWrap(glm::ivec3 wrap) {mWrap = wrap;}

      void SetUnit(GLuint unit)  {mUnit = unit;}
      GLuint GetUnit()  {return mUnit;}

      void SetSize(glm::ivec3 size);
      glm::ivec3 GetSize() {return mSize;}
      void Resize(glm::ivec3 size);

   private:
      GLuint mTexture = -1;
      GLuint mUnit = -1;
      GLenum mTarget = -1;
      GLint mLevels = 1;
      GLint mLayers = 1;
      GLint mInternalFormat = GL_RGBA32F;

      glm::ivec3 mWrap = glm::ivec3(GL_REPEAT, GL_REPEAT, GL_REPEAT);
      glm::ivec2 mFilter = glm::ivec2(GL_LINEAR, GL_NEAREST);
      glm::ivec3 mSize = glm::ivec3(0);

   friend void SwapUnits(ImageTexture& i0, ImageTexture& i1);
};
