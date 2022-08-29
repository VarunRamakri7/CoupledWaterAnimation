#version 430            

layout(location=0) uniform vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

struct aabb2D
{
   vec2 mMin;
   vec2 mMax;
};

struct PbdObj
{
   vec4 color;
   vec2 x;
   vec2 v;
   vec2 p;
   vec2 dp_t;
   vec2 dp_n;
   vec2 hw;
   float rm;
   float n;
   float e;
   float padding;
};

layout (std430, binding = 3) buffer OBJS 
{
	PbdObj mObj[];
};

out VertexData
{
   vec4 color;  
} outData; 

void main(void)
{
   int ix = gl_InstanceID;
   aabb2D box = aabb2D(mObj[ix].x - mObj[ix].hw, mObj[ix].x + mObj[ix].hw);

   vec4 verts[4];
   verts[0] = vec4(box.mMin, 0.0, 1.0);
   verts[1] = vec4(box.mMin.x, box.mMax.y, 0.0, 1.0);
   verts[2] = vec4(box.mMax.x, box.mMin.y, 0.0, 1.0);
   verts[3] = vec4(box.mMax, 0.0, 1.0);

   outData.color = mObj[ix].color;

	gl_Position = verts[gl_VertexID%4]; //transform vertices and send result into pipeline
}