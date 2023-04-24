#include "AttriblessRendering.h"
#include <GL/glew.h>


/*
Be sure to declate an attribless vao:
	
	GLuint attribless_vao = -1;

and initialize it
 
	   glGenVertexArrays(1, &attribless_vao);

before using these functions
*/

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

In vertex ahder declare:
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

