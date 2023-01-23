#ifndef __SURF_H__
#define __SURF_H__

#include <windows.h>
#include <GL/glew.h>

struct surf_vao
{
   GLuint vao = -1;
   GLenum mode;               //Primitive mode, e.g. GL_POINTS, GL_TRIANGLES, etc...
   GLuint num_vertices = 0;   //Number of vertices to draw with glDrawArrays

   void Draw() 
   {
      glDrawArrays(mode, 0, num_vertices);
   }
};

surf_vao create_surf_points_vao(int n_grid);
surf_vao create_surf_triangles_vao(int n_grid);
surf_vao create_surf_separate_points_vao(int n_grid);

surf_vao create_surf_separate_vao(int n_grid);
surf_vao create_surf_interleaved_vao(int n_grid);

struct indexed_surf_vao
{
   GLuint vao = -1;
   GLenum mode = GL_LINE;            //Primitive mode, e.g. GL_POINTS, GL_TRIANGLES, etc...
   GLuint num_indices = 0; //Number of indices to draw with glDrawElements
   GLenum type;            //Type of indices, e.g. GL_UNSIGNED_SHORT, GL_UNSIGNED_INT
   void* indices = 0;      //Offset, in bytes, from the beginning of the index buffer to start drawing from         

   void Draw() 
   {
      glDrawElements(mode, num_indices, type, indices);
   }

   void DrawInstanced()
   {
       glDrawElementsInstanced(mode, num_indices, type, indices, 9);
   }
};

indexed_surf_vao create_indexed_surf_interleaved_vao(int n_grid);
indexed_surf_vao create_indexed_surf_strip_vao(int n_grid);

#endif
