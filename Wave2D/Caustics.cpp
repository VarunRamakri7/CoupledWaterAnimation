#include "Caustics.h"
#include "InitShader.h"
#include <glm/glm.hpp>
#include <vector>


namespace CausticsUniformLocs
{
   int pass = 0;
};

namespace CausticsAttribLocs
{
   int tex_coord = 1;
};

void Caustics::ReloadShader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if(new_shader != -1) 
   {
      if (shader != -1)
      {
         glDeleteProgram(shader);
      }
      shader = new_shader;  
   }  
}

void Caustics::Init(int width, int height)
{  
   mWidth = width;
   mHeight = height;

   shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

  
   glGenTextures(1, &output_tex);
   glBindTexture(GL_TEXTURE_2D, output_tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   //Create the framebuffer object
   glGenFramebuffers(1, &fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_tex, 0);

   //unbind the fbo
   glBindFramebuffer(GL_FRAMEBUFFER, 0);


   mIndexedSurfVao.Init(width);
}

void Caustics::Render(GLuint input_tex)
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   glUseProgram(shader);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
   glViewport(0, 0, mWidth, mHeight);
   glClear(GL_COLOR_BUFFER_BIT);
   glBindVertexArray(mIndexedSurfVao.vao);
   
   //glUniform1i(BloomUniformLocs::pass, 0);
   glDrawBuffer(GL_COLOR_ATTACHMENT0);
   glBindTextureUnit(0, input_tex); //read from
   mIndexedSurfVao.Draw();

   glDisable(GL_BLEND);
   //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


GLuint Caustics::indexed_surf_vao::create_strips_index_buffer(int n_grid)
{
    //Declare a vector to hold GL_TRIANGLE_STRIP vertices, texture coordinates, and normals 
    const int num_strips = n_grid - 1;
    const int num_indices_per_strip = 2 * n_grid;
    const int num_restart_indices = num_strips;
    const int num_indices = num_strips* num_indices_per_strip + num_restart_indices;
    const unsigned int restart_index = -1;

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

    GLuint index_buffer;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer); //GL_ELEMENT_ARRAY_BUFFER is the target for buffers containing indices

    //Upload from main memory to gpu memory.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), GL_STATIC_DRAW);

    return index_buffer;
}

GLuint Caustics::indexed_surf_vao::create_indexed_surf_interleaved_vbo(int n_grid)
{
   //Declare a vector to hold GL_POINTS vertices, texture coordinates, and normals 
   const int num_vertices = n_grid*n_grid;

   std::vector<glm::vec2> surf_verts;
   surf_verts.reserve(num_vertices);

   //Insert positions
   for(int i=0; i<n_grid; i++)
   {
      for (int j=0; j<n_grid; j++)
      {
         glm::vec2 t0 = glm::vec2(float(i), float(j))/float(n_grid-1);
         surf_verts.push_back(t0);
      }
   }

   GLuint vbo;
   glGenBuffers(1, &vbo); //Generate vbo to hold vertex attributes for triangle.
   glBindBuffer(GL_ARRAY_BUFFER, vbo); //Specify the buffer where vertex attribute data is stored.
   
   //Upload from main memory to gpu memory.
   glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*surf_verts.size(), surf_verts.data(), GL_STATIC_DRAW);

   return vbo;
}

void Caustics::indexed_surf_vao::Init(int n)
{
    //Generate vao id to hold the mapping from attrib variables in shader to memory locations in vbo
    glGenVertexArrays(1, &vao);

    //Binding vao means that bindbuffer, enablevertexattribarray and vertexattribpointer state will be remembered by vao
    glBindVertexArray(vao);

    GLuint vbo = create_indexed_surf_interleaved_vbo(n);
    mode = GL_TRIANGLE_STRIP;
    const int num_strips = n - 1;
    const int num_indices_per_strip = 2 * n;
    const int num_restart_indices = num_strips;
    num_indices = num_strips * num_indices_per_strip + num_restart_indices;
    
    type = GL_UNSIGNED_INT;

    GLuint index_buffer = create_strips_index_buffer(n);
    glPrimitiveRestartIndex(-1);

    glEnableVertexAttribArray(CausticsAttribLocs::tex_coord);
    //Tell opengl how to get the attribute values out of the vbo (stride and offset).
    glVertexAttribPointer(CausticsAttribLocs::tex_coord, 2, GL_FLOAT, false, 0, 0);
 
    glBindVertexArray(0); //unbind the vao

   glEnable(GL_PRIMITIVE_RESTART);
}

void Caustics::DrawGui()
{

}
