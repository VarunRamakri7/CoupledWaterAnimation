#include "Buffer.h"
#include <vector>
#include <GL/glew.h>

Buffer::Buffer(GLuint target, GLuint binding): mTarget(target), mBinding(binding)
{

}

void Buffer::Init(int size, void* data, GLuint flags)
{
   mSize = size;
   mFlags = flags;
   if(mEnableDebug == true)
   {
      mFlags = mFlags | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;
   }
   
   if(mBuffer != -1)
   {
      glDeleteBuffers(1, &mBuffer);
   }

   glGenBuffers(1, &mBuffer);
   glBindBuffer(mTarget, mBuffer);
   glNamedBufferStorage(mBuffer, size, data, mFlags);
}

void Buffer::BufferSubData(int offset, int size, void* data)
{
   glNamedBufferSubData(mBuffer, offset, size, data);
}

void Buffer::BindBufferBase()
{
   glBindBufferBase(mTarget, mBinding, mBuffer);
}

void Buffer::DebugReadInt()
{
   if(mEnableDebug)
   {
      glBindBuffer(mTarget, mBuffer);
      int *ptr = (int*) glMapNamedBuffer(mBuffer, GL_READ_ONLY);
      std::vector<int> test;
      for (int i = 0; i < mSize/sizeof(int); i++)
      {
         test.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
}

void Buffer::DebugReadFloat()
{
   if(mEnableDebug)
   {
      glBindBuffer(mTarget, mBuffer);
      float *ptr = (float*) glMapNamedBuffer(mBuffer, GL_READ_ONLY);
      std::vector<float> test;
      for (int i = 0; i < mSize/sizeof(float); i++)
      {
         test.push_back(ptr[i]);
      }
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
   }
}
