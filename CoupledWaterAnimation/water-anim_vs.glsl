#version 440

layout(location = 0) uniform mat4 M;
layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

layout(location = 0) in vec3 pos_attrib; //this variable holds the position of mesh vertices
//layout(location = 1) in vec2 tex_coord_attrib;
//layout(location = 2) in vec3 normal_attrib;
//layout(location = 3) in vec3 color_attrib;
//layout(location = 4) in mat4 offset_attrib;

out vec3 particle_pos;

//out VertexData
//{
//	vec2 tex_coord;
//	vec3 pw;       // world-space vertex position
//	vec3 nw;   // world-space normal vector
//	vec3 color;
//} outData;

void main ()
{
    gl_Position = PV * M * vec4(pos_attrib, 1.0f);
	particle_pos = vec3(M * vec4(pos_attrib, 1.0));

	//outData.pw = vec3(M * vec4(pos_attrib, 1.0));

	//if (pass == 1)
	//{
	//	outData.tex_coord = tex_coord_attrib; // Send tex_coord to fragment shader
	//	outData.nw = vec3(M * vec4(normal_attrib, 0.0)); // World-space normal vector
	//	outData.color = color_attrib;
	//}
}