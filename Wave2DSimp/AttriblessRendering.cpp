#include "AttriblessRendering.h"
#include <GL/glew.h>

/*
Be sure to bind an attribless vao before calling draw* functions.
*/

void bind_attribless_vao()
{
	static GLuint attribless_vao = -1;
	if(attribless_vao == -1)
	{
		glGenVertexArrays(1, &attribless_vao);
	}
	glBindVertexArray(attribless_vao);
}


/*
draw_attribless_cube

In vertex shader declare:

	const vec4 cube[8] = vec4[]( vec4(-1.0, -1.0, -1.0, 1.0),
								 vec4(-1.0, +1.0, -1.0, 1.0),
								 vec4(+1.0, +1.0, -1.0, 1.0),
								 vec4(+1.0, -1.0, -1.0, 1.0),
								 vec4(-1.0, -1.0, +1.0, 1.0),
								 vec4(-1.0, +1.0, +1.0, 1.0),
								 vec4(+1.0, +1.0, +1.0, 1.0),
								 vec4(+1.0, -1.0, +1.0, 1.0));

	const int index[14] = int[](1, 0, 2, 3, 7, 0, 4, 1, 5, 2, 6, 7, 5, 4);

In vertex shader main() use:

	int ix = index[gl_VertexID];
	vec4 v = cube[ix];
	gl_Position = PV*v;

*/
void draw_attribless_cube()
{
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}


/*
draw_attribless_quad

In vertex shader declare:
const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );


In vertex shader main() use:

	gl_Position = quad[ gl_VertexID ]; //get clip space coords out of quad array

*/

void draw_attribless_quad()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // draw quad
}


/*
draw_attribless_triangle_grid

In vertex shader declare:
uniform int nx;
uniform int ny;
const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0),
										vec4(-1.0, -1.0, 0.0, 1.0),
										vec4( 1.0, 1.0, 0.0, 1.0),
										vec4( 1.0, -1.0, 0.0, 1.0) );


In vertex shader main() use:

	gl_Position = quad[ gl_VertexID ]; //get clip space coords out of quad array

*/

void draw_attribless_triangle_grid(int nx, int ny)
{
	const int GRID_UNIFORM_LOCATION = 1000;
	if(nx <= 1 || ny <= 1) return;
	//nx,ny: number of rows and columns of vertices
	int n = (nx-1)*(ny-1)*6;
	glUniform2i(GRID_UNIFORM_LOCATION, nx, ny);
	glDrawArrays(GL_TRIANGLES, 0, n);
}

