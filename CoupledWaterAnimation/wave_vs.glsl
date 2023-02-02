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
	pos.y = 4.0 * height;

	gl_Position = PV * M * offset_attrib * pos; //transform vertices and send result into pipeline

	//Use dot notation to access members of the interface block
	outData.tex_coord = tex_coord_attrib.xy;           //send tex_coord to fragment shader
	outData.pw = vec3(M * vec4(pos_attrib.xyz, 1.0));		//world-space vertex position
	outData.nw = vec3(M * vec4(normal_attrib.xyz, 0.0));	//world-space normal vector
	outData.color = color_attrib;
}

/*layout(binding = 0) uniform sampler2D diffuse_tex;

layout (location = 1000) uniform ivec2 nxy = ivec2(10, 10);
out vec2 tex_coord; 

//The rectangle that gets covered by an nxy.x x nxy.y mesh of vertices
const vec4 rect[4] = vec4[](vec4(-1.0, -1.0, 0.0, 1.0), vec4(+1.0, -1.0, 0.0, 1.0),
							vec4(-1.0, +1.0, 0.0, 1.0), vec4( +1.0, +1.0, 0.0, 1.0));

const ivec2 offset[6] = ivec2[](ivec2(0,0), ivec2(1,0), ivec2(0, 1), ivec2(1, 0), ivec2(0, 1), ivec2(1,1));

//This is just generating a grid in attributeless fashion
void grid_vertex(out vec4 pos, out vec2 uv)
{
	ivec2 qxy = nxy - ivec2(1); //number of rows and columns of quads
	int q = gl_VertexID / 6;	//1D quad index (two triangles)
	int v = gl_VertexID % 6;	//vertex index within the quad
	ivec2 ij = ivec2(q % qxy.x, q / qxy.x); //2D quad index of current vertex
	ij += offset[v]; //2D grid index of each point
	uv = ij/vec2(qxy);
	pos = mix(mix(rect[0], rect[1], uv.s), mix(rect[2], rect[3], uv.s), uv.t);
}

void main(void)
{
	vec4 pos;
	vec2 uv;
	grid_vertex(pos, uv);
	float height = textureLod(diffuse_tex, uv, 0.0).r;
	pos.z = 4.0 * height;
	gl_Position = PV * M * pos;
	tex_coord = uv;
}*/