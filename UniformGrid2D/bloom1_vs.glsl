#version 430            
layout(location = 0) uniform int pass = 0;

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

out vec2 tex_coord;

void main(void)
{
	vec4 v = quad[ gl_VertexID ];
   gl_Position = v; //get clip space coords out of quad array
	tex_coord = 0.5*v.xy+0.5;
}