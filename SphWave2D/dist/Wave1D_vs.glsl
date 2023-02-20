#version 450            

uniform float time;
uniform int uMode = 0;

out vec2 tex_coord; 

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

void main(void)
{
	gl_Position = quad[ gl_VertexID ]; //get clip space coords out of quad array
	tex_coord = 0.5*quad[ gl_VertexID ].xy + vec2(0.5);
}