#version 430
layout(binding = 0) uniform sampler2D diffuse_tex; 
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

in VertexData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
} inData;   //block is named 'inData'

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
   float h = 0.5*texture(u0, inData.tex_coord).r + 0.5;
   fragcolor = vec4(1.0);
   fragcolor = mix(vec4(1.0), vec4(0.2, 0.5, 0.9, 1.0), h);
}