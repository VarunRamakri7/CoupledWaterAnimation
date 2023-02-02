#version 450

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

layout(binding = 0) uniform sampler2D diffuse_tex;

layout(location = 0) in vec4 pos_attrib; //this variable holds the position of mesh vertices
layout(location = 1) in vec4 tex_coord_attrib;
layout(location = 2) in vec4 normal_attrib;
layout(location = 3) in vec3 color_attrib;
layout(location = 4) in mat4 offset_attrib;

out VertexData
{
	vec2 tex_coord;
	vec3 pw;       //world-space vertex position
	vec3 nw;   //world-space normal vector
	vec3 color;
} outData; 

void main(void)
{
	vec4 pos = vec4(pos_attrib.xyz, 1.0);

	float height = textureLod(diffuse_tex, tex_coord_attrib.xy, 0.0).r;
	pos.y = 50.0f * height;

	gl_Position = PV * M * offset_attrib * pos; //transform vertices and send result into pipeline

	//Use dot notation to access members of the interface block
	outData.tex_coord = tex_coord_attrib.xy;           //send tex_coord to fragment shader
	outData.pw = vec3(M * vec4(pos_attrib.xyz, 1.0));		//world-space vertex position
	outData.nw = vec3(M * vec4(normal_attrib.xyz, 0.0));	//world-space normal vector
	outData.color = color_attrib;
}