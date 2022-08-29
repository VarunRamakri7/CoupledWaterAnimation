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

layout(std140, binding = 1) uniform LightUniforms
{
   vec4 La;	//ambient light color
   vec4 Ld;	//diffuse light color
   vec4 Ls;	//specular light color
   vec4 light_w; //world-space light position
};

layout(std140, binding = 2) uniform MaterialUniforms
{
   vec4 ka;	//ambient material color
   vec4 kd;	//diffuse material color
   vec4 ks;	//specular material color
   float shininess; //specular exponent
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

