#version 430            
layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(binding = 0) uniform sampler2D u0;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
	vec4 mouse_pos;
   ivec4 mouse_buttons;
   vec4 params[2];
	vec4 pal[4];
};

layout(location = 0) in vec3 pos_attrib; //this variable holds the position of mesh vertices
layout(location = 1) in vec2 tex_coord_attrib;
layout(location = 2) in vec3 normal_attrib;  

out VertexData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
} outData; 

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

void main(void)
{
   float h = 0.1*textureLod(u0, tex_coord_attrib, 0.0).r;
	gl_Position = PV*M*vec4(tex_coord_attrib, h, 1.0); //transform vertices and send result into pipeline
   //gl_Position = PV*M*vec4(0.5, 0.5, 0.5, 1.0)*quad[ gl_VertexID ]; 	


   //Use dot notation to access members of the interface block
   outData.tex_coord = tex_coord_attrib;           //send tex_coord to fragment shader
   outData.pw = vec3(M*vec4(tex_coord_attrib, 0.0, 1.0));		//world-space vertex position
	outData.nw = vec3(M*vec4(normal_attrib, 0.0));	//world-space normal vector
}