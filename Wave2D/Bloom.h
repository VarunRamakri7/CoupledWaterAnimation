#pragma once
#include <windows.h>
#include <GL/glew.h>
#include <string>

struct Bloom
{
   Bloom();
   void Init(int width, int height);
   void ComputeBloom(GLuint input_tex);

   std::string vertex_shader = std::string("bloom_vs.glsl");
   std::string fragment_shader = std::string("bloom_fs.glsl");
   GLuint bloom_shader;

   GLuint attribless_vao = -1;
   
   GLuint fbo = -1;
   GLuint bright_tex = -1;
   GLuint blur_tex[2] = {GLuint(-1), GLuint(-1)};
   GLuint output_tex = -1;
   int mWidth = -1;
   int mHeight = -1;

};


