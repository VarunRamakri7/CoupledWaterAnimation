#pragma once
//Caustics using the wavefront technique
#include <windows.h>
#include <GL/glew.h>
#include <string>

struct Caustics
{
   std::string vertex_shader = std::string("caustics_vs.glsl");
   std::string fragment_shader = std::string("caustics_fs.glsl");
   GLuint shader=-1;

   GLuint fbo;
   GLuint output_tex;
   
   int mWidth;
   int mHeight;

   struct indexed_surf_vao
   {
      GLuint vao = -1;
      GLenum mode;            //Primitive mode, e.g. GL_POINTS, GL_TRIANGLES, etc...
      GLuint num_indices = 0; //Number of indices to draw with glDrawElements
      GLenum type;            //Type of indices, e.g. GL_UNSIGNED_SHORT, GL_UNSIGNED_INT
      void* indices = 0;      //Offset, in bytes, from the beginning of the index buffer to start drawing from         

      void Init(int n);
      GLuint create_strips_index_buffer(int n_grid);
      GLuint create_indexed_surf_interleaved_vbo(int n_grid);
      
      void Draw() 
      {
         glDrawElements(mode, num_indices, type, indices);
      }
   };

   indexed_surf_vao mIndexedSurfVao;

   void Init(int width, int height);
   void Render(GLuint input_tex);
   void DrawGui();
   void ReloadShader();
};
