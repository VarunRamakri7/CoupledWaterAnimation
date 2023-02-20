#version 450

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

layout(location = 0) in vec3 pos_attrib;

out vec3 tex_coord;

void main()
{
    tex_coord = pos_attrib;
    
    vec4 pos = PV * vec4(pos_attrib, 1.0);
    gl_Position = pos.xyww;
}  