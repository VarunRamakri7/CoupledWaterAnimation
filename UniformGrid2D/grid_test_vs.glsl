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

layout (std430, binding = 1) buffer INDICES 
{
	int index[];
};

/*
out VertexData
{
   
} outData; 
*/

const vec4 quad[4] = vec4[] (vec4(-1.0, 1.0, 0.0, 1.0), 
										vec4(-1.0, -1.0, 0.0, 1.0), 
										vec4( 1.0, 1.0, 0.0, 1.0), 
										vec4( 1.0, -1.0, 0.0, 1.0) );

void main(void)
{
/*
   int ix = index[gl_InstanceID];
   vec4 verts[4];
   verts[0] = vec4(boxes[ix].mMin, 0.0, 1.0);
   verts[1] = vec4(boxes[ix].mMin.x, boxes[ix].mMax.y, 0.0, 1.0);
   verts[2] = vec4(boxes[ix].mMax.x, boxes[ix].mMin.y, 0.0, 1.0);
   verts[3] = vec4(boxes[ix].mMax, 0.0, 1.0);

	gl_Position = verts[gl_VertexID%4]; //transform vertices and send result into pipeline
*/

   int ix = index[gl_InstanceID];
   aabb2D box = boxes[ix];
   vec2 cen = 0.5*(box.mMin+box.mMax);
   vec4 scale = vec4(0.5*(box.mMax-box.mMin), 1.0, 1.0);
   mat4 S = mat4(scale.x, 0.0, 0.0, 0.0,
                  0.0, scale.y, 0.0, 0.0,
                  0.0, 0.0, scale.z, 0.0,
                  cen.x, cen.y, 0.0, scale.w);
   gl_Position = S*quad[gl_VertexID%4];
}