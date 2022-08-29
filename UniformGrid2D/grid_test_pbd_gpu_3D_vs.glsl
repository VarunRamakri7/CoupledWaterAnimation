#version 430            

layout(location=0) uniform vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 M;
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

struct PbdObj
{
   vec4 color;
   vec4 x;
   vec4 v;
   vec4 p;
   vec4 dp_t;
   vec4 dp_n;
   vec4 hw;
   float rm;
   float n;
   float e;
   float padding;
};

layout (std430, binding = 3) buffer OBJS 
{
	PbdObj mObj[];
};


const vec4 cube[8] = vec4[] ( vec4(-1.0, -1.0, -1.0, 1.0), 
										vec4(-1.0, -1.0, +1.0, 1.0), 
										vec4(-1.0, +1.0, +1.0, 1.0), 
										vec4(-1.0, +1.0, -1.0, 1.0),
                              vec4(+1.0, -1.0, -1.0, 1.0), 
										vec4(+1.0, -1.0, +1.0, 1.0), 
										vec4(+1.0, +1.0, +1.0, 1.0), 
										vec4(+1.0, +1.0, -1.0, 1.0) );

const int cube_ix[36] = int[](0, 1, 2,    0, 2, 3,    //-x
                              4, 6, 5,    6, 4, 7,    //+x
                              1, 0, 5,    0, 5, 4,    //-y
                              2, 6, 3,    3, 6, 7,    //+y
                              4, 0, 7,    7, 0, 3,    //-z
                              5, 6, 1,    1, 6, 2);   //+z
                                                      

const vec4 normal[6] = vec4[](vec4(-1.0, 0.0, 0.0, 1.0), 
										vec4(+1.0, 0.0, 0.0, 1.0), 
										vec4(0.0, -1.0, 0.0, 1.0), 
										vec4(0.0, +1.0, 0.0, 1.0),
                              vec4(0.0, 0.0, -1.0, 1.0), 
										vec4(0.0, 0.0, +1.0, 1.0));

const int normal_ix[36] = int[]( 0, 0, 0, 0, 0, 0, //-x
                                 1, 1, 1, 1, 1, 1, //+x
                                 2, 2, 2, 2, 2, 2, //-y
                                 3, 3, 3, 3, 3, 3, //+y
                                 4, 4, 4, 4, 4, 4, //-z
                                 5, 5, 5, 5, 5, 5);//+z

out VertexData
{
   vec4 pw;
   vec4 nw;
   vec4 no;
   vec4 color;  
   vec4 po;
   vec4 p;
   flat int instanceId;
} outData;

void main(void)
{
   int ix = gl_InstanceID; //object index
   outData.instanceId = ix;
   
   vec4 cen = mObj[ix].x;
   vec4 scale = vec4(mObj[ix].hw.xyz, 1.0);
   mat4 S = mat4(scale.x, 0.0, 0.0, 0.0,
                  0.0, scale.y, 0.0, 0.0,
                  0.0, 0.0, scale.z, 0.0,
                  cen.x, cen.y, cen.z, scale.w);

   int vx = cube_ix[gl_VertexID%36]; //vertex index
   outData.p = cube[vx];
   outData.po = S*cube[vx];
   outData.pw = M*outData.po;
   gl_Position = PV*outData.pw;

   int nx = normal_ix[gl_VertexID%36]; //normal index
   outData.no = normal[nx];
   outData.nw = M*outData.no;
   outData.color = mObj[ix].color;
}