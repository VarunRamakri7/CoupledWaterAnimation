#version 430
layout(binding = 0) uniform sampler2D u0;
layout(binding = 1) uniform sampler2D u1;
layout(binding = 2) uniform sampler2D u2;
layout(location = 2) uniform vec4 slider = vec4(0.0);

layout (vertices = 3) out;  //number of output verts of the tess. control shader

in VertexData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
   vec2 d2u;
} inData[]; 

out TessControlData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector

} outData[]; 

float tess_level(sampler2D tex, vec2 tex_coord);
float tess_level_d2u(vec2 d2u);

void main()
{

	//Pass-through: just copy input vertices to output
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

   outData[gl_InvocationID].tex_coord = inData[gl_InvocationID].tex_coord;
   outData[gl_InvocationID].pw = inData[gl_InvocationID].pw;
   outData[gl_InvocationID].nw = inData[gl_InvocationID].nw;

   //set tessellation levels
   if(gl_InvocationID==0)
   {
      vec2 mid = 0.333333*(gl_in[0].gl_Position.xy + gl_in[1].gl_Position.xy + gl_in[2].gl_Position.xy);
	   gl_TessLevelInner[0] = (tess_level(u0, mid) + tess_level(u1, mid) + tess_level(u2, mid))/3.0;
   }

   float ta = tess_level_d2u(inData[(gl_InvocationID+1)%3].d2u);
   float tb = tess_level_d2u(inData[(gl_InvocationID+2)%3].d2u);
   gl_TessLevelOuter[gl_InvocationID] = max(ta, tb);
}

float tess_level_d2u(vec2 d2u)
{
   vec4 s = vec4(0.014085, 0.253521, 0.802817, 0.027027);
   float L = s.z*(d2u.x + d2u.y);
   float tess = mix(2.0, 8.0, smoothstep(s.x*s.w, s.y*s.w, L));
   return tess;
}

float tess_level(sampler2D tex, vec2 tex_coord)
{
   //vec2 size = textureSize(tex, 0);
   //vec3 step = vec3(1.0/size, 0.0);   

   //float c = textureLod(tex, tex_coord, 0.0).r;
   //float n = textureLod(tex, tex_coord+step.zy, 0.0).r;
   //float s = textureLod(tex, tex_coord-step.zy, 0.0).r;
   //float e = textureLod(tex, tex_coord+step.xz, 0.0).r;
   //float w = textureLod(tex, tex_coord-step.xz, 0.0).r;
   
   //vec2 g = vec2(abs(2.0*c-n-s), abs(2.0*c-e-w));

   vec4 ua = textureGather(tex, tex_coord, 0);
   vec4 ub = textureGatherOffset(tex, tex_coord, ivec2(-1, -1), 0);
   float d2y = 2.0*ua.w-ua.x-ub.z;
   float d2x = 2.0*ua.w-ua.z-ub.x;
   vec2 g = vec2(abs(d2x), abs(d2y));
   return tess_level_d2u(g);
}