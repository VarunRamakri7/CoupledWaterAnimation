#version 440

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

in vec3 pos_attrib; //this variable holds the position of mesh vertices
in vec2 tex_coord_attrib;
in vec3 normal_attrib;  

out vec2 tex_coord; 

void main(void)
{
	gl_Position = PV * M * vec4(pos_attrib, 1.0); //transform vertices and send result into pipeline
	tex_coord = tex_coord_attrib; //send tex_coord to fragment shader
}