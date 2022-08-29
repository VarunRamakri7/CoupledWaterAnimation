#version 430            
layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;
layout(location = 2) uniform int pass = 0;

layout(std140, binding = 0) uniform SceneUniforms
{
	mat4 PV;
	vec4 mouse_pos;
   ivec4 mouse_buttons;
   vec4 params[2];
	vec4 pal[4];
};



const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

void main(void)
{
   gl_Position = quad[ gl_VertexID ]; //get clip space coords out of quad array
}