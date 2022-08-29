#pragma once
#include <windows.h>
#include <GL/glew.h>

//Contents of the indirect buffer
struct DrawElementsIndirectCommand
{
   GLuint count = 0;
   GLuint instanceCount = 1;
   GLuint firstIndex = 0;
   GLuint baseVertex = 0;
   GLuint baseInstance = 0;
};

//Wrapper for an indirect draw call
struct DrawElementsIndirect
{
   GLenum mode = GL_TRIANGLES;
   GLenum type = GL_UNSIGNED_INT;
   GLvoid* indirect = 0;

   GLuint indirect_buffer = -1;
   GLuint vao = -1;
   DrawElementsIndirectCommand indirect_data;

   void Init();

   inline void BindVao()
   {
      glBindVertexArray(vao);
   }

   inline void BindIndirect()
   {
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer);  
   }

   inline void Draw()
   {
      glDrawElementsIndirect(mode, type, indirect);
      //debug
      //glDrawElements(mode, indirect_data.count, type, 0);
   }

   void BindAllAndDraw()
   {
      BindIndirect();
      BindVao();
      Draw();
   }
};

struct GridMesh
{
   GLuint vbo = -1;
   GLuint index_buffer = -1;

   GLuint count = -1;//number of indices

   void Init();
   GLuint StripsIndexBuffer(int n_grid);
   GLuint TrianglesIndexBuffer(int n_grid);
   GLuint IndexedVbo(int n_grid);
   void VertexAttribPointer(GLint loc);
};

DrawElementsIndirect CreateIndirectGridMeshStrips(int n);
DrawElementsIndirect CreateIndirectGridMeshTriangles(int n);
