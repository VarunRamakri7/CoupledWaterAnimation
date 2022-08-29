#version 430 
layout(location = 0) uniform mat4 M;
layout(binding = 0) uniform sampler2D u0;

//layout (triangles, equal_spacing, ccw) in;
//Try some of these other options
//layout (triangles, fractional_odd_spacing, ccw) in;
layout (triangles, fractional_even_spacing, ccw) in;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
	vec4 mouse_pos;
   ivec4 mouse_buttons;
   vec4 params[2];
	vec4 pal[4];
};

in TessControlData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
} inData[]; 

out TessEvalData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
} outData; 

void main()
{
	const float u = gl_TessCoord[0];
	const float v = gl_TessCoord[1];
	const float w = gl_TessCoord[2];

	const vec4 p0 = gl_in[0].gl_Position;
	const vec4 p1 = gl_in[1].gl_Position;
	const vec4 p2 = gl_in[2].gl_Position;

	vec4 p = u*p0 + v*p1 + w*p2;

	float h = 0.1*textureLod(u0, p.xy, 0.0).r;
	p.z = h;

	gl_Position = PV*M*p;

	outData.tex_coord = u*inData[0].tex_coord + v*inData[1].tex_coord + w*inData[2].tex_coord;
	outData.pw = u*inData[0].pw + v*inData[1].pw + w*inData[2].pw;
	outData.nw = u*inData[0].nw + v*inData[1].nw + w*inData[2].nw;
}
