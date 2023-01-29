#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp> //for pi
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Surf.h"

//Buffer offset casts an integer to a pointer so that it can be passed as 
// the last arg to glVertexAttribPointer

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define RESTART_INDEX 65535

namespace AttribLocs
{
    int pos = 0;
    int tex_coord = 1;
    int normal = 2;
	int colors = 3;
	int offsets = 4;
}

//The surface to draw.
float sinc2D(const glm::vec2& p)
{
    const float eps = 1e-12f;
    float r = glm::length(p);
    float z = 1.0f;

    if (r < eps)
    {
        return 1.0f;
    }

    return z = sin(r) / r;
}

glm::vec3 surf(glm::mat3& M, int i, int j)
{
    glm::vec3 p = M * glm::vec3(float(i), float(j), 1.0f);
    return glm::vec3(p.x, p.y, 10.0f * sinc2D(p));
}

//Compute normal using finite differences
glm::vec3 normal(glm::mat3& M, int i, int j)
{
    glm::vec3 px0 = surf(M, i - 1, j);
    glm::vec3 px1 = surf(M, i + 1, j);

    glm::vec3 py0 = surf(M, i, j - 1);
    glm::vec3 py1 = surf(M, i, j + 1);

    glm::vec3 dFdx = 0.5f * (px1 - px0);
    glm::vec3 dFdy = 0.5f * (py1 - py0);

    return glm::normalize(glm::cross(dFdx, dFdy));
}

/// <summary>
/// Genetate VBO for the non-instanced attributes of the surface drawn as an indexed triangle strip
/// </summary>
/// <param name="n_grid"></param>
/// <returns>VBO ID</returns>
GLuint create_indexed_strip_surf_vbo(int n_grid)
{
    const int num_vertices = n_grid * n_grid;

    std::vector<float> surf_verts;
    surf_verts.reserve((3 + 2 + 3) * num_vertices);

    //Create matrix to transform indices to points
    glm::mat3 T = glm::translate(glm::mat3(1.0f), glm::vec2(-float(n_grid) / 2.0, -float(n_grid) / 2.0));
    glm::mat3 S = glm::scale(glm::mat3(1.0f), glm::vec2(20.0f / n_grid));
    glm::mat3 M = S * T;

    //Insert positions
    for (int i = 0; i < n_grid; i++)
    {
        for (int j = 0; j < n_grid; j++)
        {
            glm::vec3 p0 = surf(M, i, j);
            glm::vec2 t0 = glm::vec2(float(i), float(j)) / float(n_grid - 1);
            glm::vec3 n0 = normal(M, i, j);
            //float inst = i;

            //Insert triangles
            surf_verts.push_back(p0.x);   surf_verts.push_back(p0.y); surf_verts.push_back(p0.z);
            surf_verts.push_back(t0.x);   surf_verts.push_back(t0.y);
            surf_verts.push_back(n0.x);   surf_verts.push_back(n0.y); surf_verts.push_back(n0.z);
        }
    }

    GLuint vbo;
    glGenBuffers(1, &vbo); //Generate vbo to hold vertex attributes for triangle.
    glBindBuffer(GL_ARRAY_BUFFER, vbo); //Specify the buffer where vertex attribute data is stored.

    //Upload from main memory to gpu memory.
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * surf_verts.size(), surf_verts.data(), GL_STATIC_DRAW);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vbo;
}

/// <summary>
/// Genetate colors VBO for the instanced attributes of the surface
/// </summary>
/// <returns>VBO IDs</returns>
GLuint create_instanced_colors_vbo()
{
	std::vector<glm::vec3> colors;

	for (int i = 0; i < 9; i++)
	{
		float coli = (i * 0.1f) / 2.0f;
		colors.push_back(glm::vec3(0.1f + coli, 0.1f + coli, i / 9));
	}

	GLuint colorVBO; // For offsets and colors
	glGenBuffers(1, &colorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO); // Bind colors
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * colors.size(), colors.data(), GL_STATIC_DRAW); // Send data to server

	return colorVBO;
}

/// <summary>
/// Genetate offsets VBO for the instanced attributes of the surface
/// </summary>
/// <returns>VBO IDs</returns>
GLuint create_instanced_offsets_vbo()
{
	std::vector<glm::mat4> offsets;

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			glm::mat4 trans = glm::mat4(1.0f);
			//trans *= glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f))* glm::scale(glm::vec3(1.0f));
			offsets.push_back(glm::translate(trans, glm::vec3(30.0f * i, 30.0f * j, 0.0f)));
		}
	}

	GLuint offsetsVBO; // For offsets and colors
	glGenBuffers(1, &offsetsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, offsetsVBO); // Bind colors
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * offsets.size(), offsets.data(), GL_STATIC_DRAW); // Send data to server

	return offsetsVBO;
}

/// <summary>
/// Create Index Buffer to draw surface as Triangle strip
/// </summary>
/// <param name="n_grid">Size of Grid</param>
/// <returns>Index Buffer ID</returns>
GLuint create_strip_index_buffer(unsigned int n_grid)
{
    const int num_indices = n_grid * n_grid;

    //Declare a vector to hold GL_POINTS vertices, texture coordinates, and normals 
    std::vector<unsigned int> surf_indices;
    surf_indices.reserve(num_indices);

    //Insert indices in order to make a triangle strip
    for (unsigned int i = 0; i < n_grid - 1; i++)
    {
        for (int j = 0; j < n_grid - 1; j++)
        {
            //Insert indices
            surf_indices.push_back((unsigned int)(i * n_grid + j));
            surf_indices.push_back((unsigned int)((i + 1) * n_grid + j));
        }
        surf_indices.push_back((unsigned int)RESTART_INDEX); // Insert restart index
    }

    GLuint index_buffer;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer); //GL_ELEMENT_ARRAY_BUFFER is the target for buffers containing indices

    //Upload from main memory to gpu memory.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * surf_indices.size(), surf_indices.data(), GL_STATIC_DRAW);

	// glBindBuffer(GL_ARRAY_BUFFER, 0);

    return index_buffer;
}

/// <summary>
/// Create VAO to draw the surface as an indexed triangle strip
/// </summary>
/// <param name="n_grid"></param>
/// <returns>Indexed surface</returns>
indexed_surf_vao create_indexed_surf_strip_vao(int n)
{
    indexed_surf_vao surf;

    //Generate vao id to hold the mapping from attrib variables in shader to memory locations in vbo
    glGenVertexArrays(1, &surf.vao);

    //Binding vao means that bindbuffer, enablevertexattribarray and vertexattribpointer state will be remembered by vao
    glBindVertexArray(surf.vao);

    GLuint vbo = create_indexed_strip_surf_vbo(n);

    surf.mode = GL_TRIANGLE_STRIP;
    const int num_triangles = (n - 1) * (n - 1) * 2;
    surf.num_indices = num_triangles * 3;
    surf.type = GL_UNSIGNED_INT;

    GLuint index_buffer = create_strip_index_buffer((unsigned int) n);

    //Separate pos, tex_coord and normal in vbo
    glEnableVertexAttribArray(AttribLocs::pos);
    glEnableVertexAttribArray(AttribLocs::tex_coord);
    glEnableVertexAttribArray(AttribLocs::normal);

	// Instanced attributes
	//glEnableVertexAttribArray(AttribLocs::offsets);
	//glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[0]);

    //Tell opengl how to get the attribute values out of the vbo (stride and offset).
    const int stride = (3 + 2 + 3) * sizeof(float);
    glVertexAttribPointer(AttribLocs::pos, 3, GL_FLOAT, false, stride, BUFFER_OFFSET(0));
    glVertexAttribPointer(AttribLocs::tex_coord, 2, GL_FLOAT, false, stride, BUFFER_OFFSET(3 * sizeof(float)));
    glVertexAttribPointer(AttribLocs::normal, 3, GL_FLOAT, false, stride, BUFFER_OFFSET((3 + 2) * sizeof(float)));

	GLuint offsetsVBP = create_instanced_offsets_vbo();
	for (int i = 0; i < 4; i++)
	{
		glVertexAttribPointer(AttribLocs::offsets + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), BUFFER_OFFSET(sizeof(glm::vec4) * i));
		glEnableVertexAttribArray(AttribLocs::offsets + i);
		glVertexAttribDivisor(AttribLocs::offsets + i, 1);
	}
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint colorVBO = create_instanced_colors_vbo();
	glEnableVertexAttribArray(AttribLocs::colors);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glVertexAttribPointer(AttribLocs::colors, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(0));
	glVertexAttribDivisor(AttribLocs::colors, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0); //unbind the vao

    return surf;
}