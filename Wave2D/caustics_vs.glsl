#version 430            
layout(location = 0) uniform int pass = 0;
layout(binding = 0) uniform sampler2D input;

layout(location = 1) in vec2 tex_coord_attrib;

out vec2 tex_coord;
out vec3 p0;
out vec3 p1;
out vec3 r;
out vec3 debug;

ivec2 clamp_coord(ivec2 coord);
float plaIntersect( in vec3 ro, in vec3 rd, in vec4 p );

void main(void)
{
   vec2 p = 2.0*tex_coord_attrib-vec2(1.0);
   gl_Position = vec4(p, 0.0, 1.0);
	tex_coord = tex_coord_attrib;

   ivec2 coord = ivec2(tex_coord*textureSize(input, 0));
   const float hscale = 0.1;
   float h = hscale*texelFetch(input, coord+ivec2(1, 0), 0).r;
   //float h = textureLod(input, tex_coord, 0.0).r;
   float dhdx = hscale*(texelFetch(input, clamp_coord(coord+ivec2(1, 0)), 0).r - texelFetch(input, clamp_coord(coord-ivec2(1, 0)), 0).r);
   float dhdy = hscale*(texelFetch(input, clamp_coord(coord+ivec2(0, 1)), 0).r - texelFetch(input, clamp_coord(coord-ivec2(0, 1)), 0).r);

   vec3 n = normalize(vec3(-dhdx, -dhdy, 2.0));

   r = refract(vec3(0.0, 0.0, -1.0), n, 1.0/1.33);
   //r = normalize(r); //needed?

   p0 = vec3(p, 0.0);
   p1 = vec3(p, h);

   vec4 pla = vec4(0.0, 0.0, 1.0, +4.0);
   float t = plaIntersect(p1, r, pla);
   p1 = p0 + t*r;

   gl_Position = vec4(p1.xy + r.xy/r.z, 0.0, 1.0);
   debug = r;
}

float plaIntersect(in vec3 ro, in vec3 rd, in vec4 p )
{
    return -(dot(ro,p.xyz)+p.w)/dot(rd,p.xyz);
}

ivec2 clamp_coord(ivec2 coord)
{
   ivec2 size = textureSize(input, 0);
   return ivec2(clamp(coord, ivec2(0), size-ivec2(1)));
}