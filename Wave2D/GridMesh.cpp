#include "GridMesh.h"
#include <glm/glm.hpp>
#include <vector>

namespace IndirectAttribLocs
{
   int tex_coord = 1;
};

void DrawElementsIndirect::Init()
{
   if(indirect_buffer!=-1)
   {
      glDeleteBuffers(1, &indirect_buffer);
   }
   glGenBuffers(1, &indirect_buffer);
   glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer);

   const GLuint flags = 0;
   glNamedBufferStorage(indirect_buffer, sizeof(DrawElementsIndirectCommand), &indirect_data, flags);
   //debug
   //glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand), &indirect_data, GL_STATIC_DRAW);

   if(vao!=-1)
   {
      glDeleteVertexArrays(1, &vao);
   }
   glGenVertexArrays(1, &vao);

}


void GridMesh::Init()
{
   

  
}

GLuint GridMesh::StripsIndexBuffer(int n_grid)
{
   const int num_strips = n_grid - 1;
   const int num_indices_per_strip = 2 * n_grid;
   const int num_restart_indices = num_strips;
   const int num_indices = num_strips* num_indices_per_strip + num_restart_indices;
   const unsigned int restart_index = -1;

   glEnable(GL_PRIMITIVE_RESTART);
   glPrimitiveRestartIndex(restart_index);

   count = num_indices;

   //Declare a vector to hold GL_TRIANGLE_STRIP indices
   std::vector<unsigned int> surf_indices;
   surf_indices.reserve(num_indices);

   //Insert indices. Consider a grid of squares. We insert the two triangles that make up the square starting 
   // with index idx.
   unsigned int idx = 0;
   for (int i = 0; i < n_grid - 1; i++)
   {
      for (int j = 0; j < n_grid; j++)
      {
         surf_indices.push_back(idx);
         surf_indices.push_back(idx + n_grid);

         idx++;
      }
      surf_indices.push_back(restart_index);
   }

   if(index_buffer!=-1)
   {
      glDeleteBuffers(1, &index_buffer);
   }
   glGenBuffers(1, &index_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer); //GL_ELEMENT_ARRAY_BUFFER is the target for buffers containing indices

   //Upload from main memory to gpu memory.
   const GLuint flags = 0;
   glNamedBufferStorage(index_buffer, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), flags);
   //debug
   //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), GL_STATIC_DRAW);

   return index_buffer;
}

GLuint GridMesh::TrianglesIndexBuffer(int n_grid)
{
   const int num_indices = (n_grid-1)*(n_grid-1)*2*3;
   count = num_indices;

   //Declare a vector to hold GL_TRIANGLE_STRIP indices
   std::vector<unsigned int> surf_indices;
   surf_indices.reserve(num_indices);

   //Insert indices. Consider a grid of squares. We insert the two triangles that make up the square starting 
   // with index idx.
   unsigned int idx = 0;
   for (int i = 0; i < n_grid - 1; i++)
   {
      for (int j = 0; j < n_grid - 1; j++)
      {
         //Insert triangle 1
         surf_indices.push_back(idx);
         surf_indices.push_back(idx+n_grid);   
         surf_indices.push_back(idx+n_grid+1);

         //Insert triangle 2
         surf_indices.push_back(idx);
         surf_indices.push_back(idx+n_grid+1);   
         surf_indices.push_back(idx+1);
      
         idx++;
      }
      idx++;
   }

   if(index_buffer!=-1)
   {
      glDeleteBuffers(1, &index_buffer);
   }
   glGenBuffers(1, &index_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer); //GL_ELEMENT_ARRAY_BUFFER is the target for buffers containing indices

   //Upload from main memory to gpu memory.
   const GLuint flags = 0;
   glNamedBufferStorage(index_buffer, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), flags);
   //debug
   //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), GL_STATIC_DRAW);

   return index_buffer;
}

GLuint GridMesh::IndexedVbo(int n_grid)
{
   const int num_vertices = n_grid*n_grid;
   //Declare a vector to hold grid texture coordinates 
   std::vector<glm::vec2> surf_verts;
   surf_verts.reserve(num_vertices);

   //Insert tex coored
   for(int i=0; i<n_grid; i++)
   {
      for (int j=0; j<n_grid; j++)
      {
         glm::vec2 t = glm::vec2(float(i), float(j))/float(n_grid-1);
         surf_verts.push_back(t);
      }
   }

   if(vbo!=-1)
   {
      glDeleteBuffers(1, &vbo);
   }
   glGenBuffers(1, &vbo); //Generate vbo to hold vertex attributes
   glBindBuffer(GL_ARRAY_BUFFER, vbo); //Specify the buffer where vertex attribute data is stored.
   
   //Upload from main memory to gpu memory.
   const GLuint flags = 0;
   glNamedBufferStorage(vbo, sizeof(glm::vec2)*surf_verts.size(), surf_verts.data(), flags);
   //debug
   //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*surf_verts.size(), surf_verts.data(), GL_STATIC_DRAW);
   return vbo;
}

void GridMesh::VertexAttribPointer(GLint loc)
{
   glEnableVertexAttribArray(loc);
   //Tell opengl how to get the attribute values out of the vbo (stride and offset).
   glVertexAttribPointer(loc, 2, GL_FLOAT, false, 0, 0);
}


DrawElementsIndirect CreateIndirectGridMeshStrips(int n)
{
   GridMesh grid_mesh;
   grid_mesh.StripsIndexBuffer(n);
   grid_mesh.IndexedVbo(n);

   DrawElementsIndirect draw;
   draw.mode = GL_TRIANGLE_STRIP;
   draw.type = GL_UNSIGNED_INT;
   draw.indirect_data.count = grid_mesh.count;

   draw.Init();

   //Bind grid_data buffers to indirect vao
   draw.BindVao();
   glBindBuffer(GL_ARRAY_BUFFER, grid_mesh.vbo); 
   grid_mesh.VertexAttribPointer(IndirectAttribLocs::tex_coord);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_mesh.index_buffer);

   return draw;
}

DrawElementsIndirect CreateIndirectGridMeshTriangles(int n)
{
   GridMesh grid_mesh;
   grid_mesh.TrianglesIndexBuffer(n);
   grid_mesh.IndexedVbo(n);

   DrawElementsIndirect draw;
   draw.mode = GL_TRIANGLES;
   draw.type = GL_UNSIGNED_INT;
   draw.indirect_data.count = grid_mesh.count;

   draw.Init();

   //Bind grid_data buffers to indirect vao
   draw.BindVao();
   glBindBuffer(GL_ARRAY_BUFFER, grid_mesh.vbo); 
   grid_mesh.VertexAttribPointer(IndirectAttribLocs::tex_coord);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_mesh.index_buffer);

   return draw;
}