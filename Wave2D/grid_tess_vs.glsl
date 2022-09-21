#version 430            
layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(binding = 0) uniform sampler2D u0;
layout(binding = 1) uniform sampler2D u1;
layout(binding = 2) uniform sampler2D u2;

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
   vec2 d2u;
} outData; 

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

vec2 get_d2u(sampler2D tex);

void main(void)
{
	gl_Position = vec4(tex_coord_attrib, 0.0, 1.0); //transform vertices and send result into pipeline
   
	//Use dot notation to access members of the interface block
	outData.tex_coord = tex_coord_attrib;           //send tex_coord to fragment shader
	outData.pw = vec3(M*vec4(tex_coord_attrib, 0.0, 1.0));		//world-space vertex position
	outData.nw = vec3(M*vec4(normal_attrib, 0.0));	//world-space normal vector
	outData.d2u = (get_d2u(u0) + get_d2u(u1) + get_d2u(u2))/3.0;
}

vec2 get_d2u(sampler2D tex)
{
   //vec2 size = textureSize(tex, 0);
   //vec3 step = vec3(1.0/size, 0.0);   

   //float c = textureLod(tex, tex_coord_attrib, 0.0).r;
   //float n = textureLod(tex, tex_coord_attrib+step.zy, 0.0).r;
   //float s = textureLod(tex, tex_coord_attrib-step.zy, 0.0).r;
   //float e = textureLod(tex, tex_coord_attrib+step.xz, 0.0).r;
   //float w = textureLod(tex, tex_coord_attrib-step.xz, 0.0).r;

   //float d2y = 2.0*c-n-s;
   //float d2x = 2.0*c-e-w;

   vec4 ua = textureGather(tex, tex_coord_attrib, 0);
   vec4 ub = textureGatherOffset(tex, tex_coord_attrib, ivec2(-1, -1), 0);

   float d2y = 2.0*ua.w-ua.x-ub.z;
   float d2x = 2.0*ua.w-ua.z-ub.x;

   return vec2(abs(d2x), abs(d2y));
}