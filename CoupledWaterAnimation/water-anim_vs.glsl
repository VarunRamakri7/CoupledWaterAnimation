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

out vec3 particle_pos;


void main ()
{
    gl_Position = PV * M * vec4(pos_attrib, 1.0f);
	particle_pos = vec3(M * vec4(pos_attrib, 1.0));
}