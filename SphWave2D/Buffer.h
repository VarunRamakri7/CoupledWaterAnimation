#pragma once

#include <GL/glew.h>

class Buffer
{
   public:
      GLuint mBuffer = -1;
      GLuint mBinding = -1;
      GLenum mTarget = -1;
      GLuint mFlags = -1;
      GLuint mSize = 0;
      bool mEnableDebug = false;
      
      Buffer(GLuint target, GLuint binding = -1);
      void Init(int size, void* data = 0, GLuint flags = 0);
      void BufferSubData(int offset, int size, void* data);
      void BindBufferBase();

      void DebugReadFloat(); //to do: make template
      void DebugReadInt();

      friend void SwapBindings(Buffer& b0, Buffer& b1);
};
