#version 430            

layout(location=0) uniform vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

struct aabb2D
{
   vec2 mMin;
   vec2 mMax;
};

layout (std430, binding = 0) buffer BOXES 
{
	aabb2D boxes[];
};

layout (std430, binding = 1) buffer COLOR 
{
	vec4 colors[];
};


out VertexData
{
   vec4 color;  
} outData; 

void main(void)
{
   int ix = gl_InstanceID;
   vec4 verts[4];
   verts[0] = vec4(boxes[ix].mMin, 0.0, 1.0);
   verts[1] = vec4(boxes[ix].mMin.x, boxes[ix].mMax.y, 0.0, 1.0);
   verts[2] = vec4(boxes[ix].mMax.x, boxes[ix].mMin.y, 0.0, 1.0);
   verts[3] = vec4(boxes[ix].mMax, 0.0, 1.0);

   outData.color = colors[ix];

	gl_Position = verts[gl_VertexID%4]; //transform vertices and send result into pipeline
}