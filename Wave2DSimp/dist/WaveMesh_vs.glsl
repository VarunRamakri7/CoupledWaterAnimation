#version 450            

layout (location=0) uniform mat4 PVM;
layout (location=1) uniform float time;
layout(binding = 0) uniform sampler2D diffuse_tex;

layout (location=1000) uniform ivec2 nxy = ivec2(10, 10);
out vec2 tex_coord; 

//The rectangle that gets covered by an nxy.x x nxy.y mesh of vertices
const vec4 rect[4] = vec4[](vec4(-1.0, -1.0, 0.0, 1.0), vec4(+1.0, -1.0, 0.0, 1.0),
							vec4(-1.0, +1.0, 0.0, 1.0), vec4( +1.0, +1.0, 0.0, 1.0));

const ivec2 offset[6] = ivec2[](ivec2(0,0), ivec2(1,0), ivec2(0, 1), ivec2(1, 0), ivec2(0, 1), ivec2(1,1));

//This is just generating a grid in attributeless fashion
void grid_vertex(out vec4 pos, out vec2 uv)
{
	ivec2 qxy = nxy - ivec2(1); //number of rows and columns of quads
	int q = gl_VertexID/6;	//1D quad index (two triangles)
	int v = gl_VertexID%6;	//vertex index within the quad
	ivec2 ij = ivec2(q%qxy.x, q/qxy.x); //2D quad index of current vertex
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
	pos.z = 4.0*height;
	gl_Position = PVM*pos;
	tex_coord = uv;
}