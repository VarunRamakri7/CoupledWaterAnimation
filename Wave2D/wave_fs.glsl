#version 430
layout(binding = 0) uniform sampler2D u0;
layout(binding = 1) uniform sampler2D u1;
layout(binding = 2) uniform sampler2D u2;

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;
layout(location = 3) uniform int iteration = 0;

//wave equation parameters
layout(location = 4) uniform float lambda = 0.04;//0.04;
layout(location = 5) uniform float atten = 0.9995; //attenuation
layout(location = 6) uniform float beta = 0.001; //damping
layout(location = 7) uniform int source = 0;
layout(location = 8) uniform int obstacle = 0; 
layout(location = 9) uniform int brush = 0; 

layout(pixel_center_integer) in vec4 gl_FragCoord;


layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 mouse_pos;
   ivec4 mouse_buttons;
   vec4 params[2];
   vec4 pal[4];
};


out vec4 fragcolor; //the output color for this fragment   
vec4 obstacle_color = vec4(-1.0, -1.0, -1.0, 1.0); 


//Performs GL_WRAP on integer coordinates
ivec2 wrap_coord(ivec2 coord);
ivec2 clamp_coord(ivec2 coord);

vec4 palette( in float t, in vec4 a, in vec4 b, in vec4 c, in vec4 d );

struct neighborhood
{
   vec4 c;
   vec4 c2;
   vec4 n;
   vec4 s;
   vec4 e;
   vec4 w;
};

neighborhood get_wrap(ivec2 coord);
neighborhood get_clamp(ivec2 coord);
neighborhood get_zero(ivec2 coord);

vec4 wave(neighborhood n);
vec4 diffusion(neighborhood n);
vec4 reaction_diffusion(neighborhood n);

float get_obstacle(vec2 coord);
void obstacle_n(vec2 coord, inout neighborhood n);

void add_source(inout vec4 fragcolor);
void add_brush(inout vec4 fragcolor);

void main(void)
{   
   if(pass == 0)
   {
      ivec2 coord = ivec2(gl_FragCoord);
      //neighborhood n = get_clamp(coord);
      neighborhood n = get_zero(coord);
      obstacle_n(coord, n);

      vec4 w = atten*wave(n);
      //vec4 w = diffusion(n);
      //vec4 w = reaction_diffusion(n);
      
      fragcolor = w;

      //animated source of waves
      add_source(fragcolor);

      //paint into the field with mouse
      add_brush(fragcolor);

      float obs = get_obstacle(gl_FragCoord.xy);
      fragcolor = mix(fragcolor, obstacle_color, obs);

   }
   if(pass == 1)
   {
      vec4 f = texelFetch(u0, ivec2(gl_FragCoord), 0);

      fragcolor = vec4(0.5*f.r+0.5);
      //fragcolor = smoothstep(0.45, 0.55, fragcolor);
      //fragcolor = pow(fragcolor, vec4(1.0/2.2));
      return;
      

      //float val = f.r;
      //fragcolor = palette(val, pal[0], pal[1], pal[2], pal[3]);
      //fragcolor = pow(fragcolor, vec4(1.0/2.2));

      //float h = f.r;
      //float dhdx = dFdx(h);
      //float dhdy = dFdy(h);
   
      float dhdx = texelFetch(u0, ivec2(gl_FragCoord.xy+vec2(1.0, 0.0)), 0).r - texelFetch(u0, ivec2(gl_FragCoord.xy-vec2(1.0, 0.0)), 0).r;
      float dhdy = texelFetch(u0, ivec2(gl_FragCoord.xy+vec2(0.0, 1.0)), 0).r - texelFetch(u0, ivec2(gl_FragCoord.xy-vec2(0.0, 1.0)), 0).r;

      vec3 n = normalize(vec3(-dhdx, -dhdy, 0.5));
      vec3 l = vec3(.707, .707, .707);
      float diff = 0.5*max(0.0, dot(n,l)) + 0.5;

      vec3 v = vec3(0.0, 0.0, 1.0);
      vec3 h = normalize(v + l);
      float shininess = 15.0;
      float spec = pow(max(0.0, dot(n,h)), shininess);
      
      fragcolor = diff*pal[0] + spec*vec4(1.0);
      //fragcolor = pow(fragcolor, vec4(1.0/2.2));

      float obs = get_obstacle(gl_FragCoord.xy);
      fragcolor = mix(fragcolor, obstacle_color, obs);
      
   }
}

vec4 palette( in float t, in vec4 a, in vec4 b, in vec4 c, in vec4 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

ivec2 wrap_coord(ivec2 coord)
{
   ivec2 size = textureSize(u1, 0);
   return ivec2(mod(coord, size));
}

ivec2 mirror(ivec2 a)
{
   ivec2 m = a;
   if(a.x<0) m.x = -(1+a.x);
   if(a.y<0) m.y = -(1+a.y);
   return m;
}

ivec2 clamp_coord(ivec2 coord)
{
   ivec2 size = textureSize(u1, 0);
   return ivec2(clamp(coord, ivec2(0), size-ivec2(1)));

   //mirrored repeat
   //ivec2 coord_mod = ivec2(coord.x%(2*size.x), coord.y%(2*size.y));
   //return (size-ivec2(1))-mirror(coord_mod-size);
}

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

//function that returns 1.0 at obstacle and 0.0 elsewhere
float get_obstacle(vec2 pos)
{
   const float hw = 0.0;

   if(obstacle==0)
   {
      return 0.0;
   }

   else if(obstacle==1)
   {
      const float r = 101.5;
      float d1 = distance(pos, vec2(512.5))-r;
      float d2 = 510.0-distance(pos, vec2(512.5));
      float d = min(d1, d2);
      return smoothstep(hw, -hw, d);
   }

   else if(obstacle==2)
   {
      float d1 = sdBox(pos-vec2(0.0, 512.0), vec2(450.0, 20.0));
      float d2 = sdBox(pos-vec2(1024.0, 512.0), vec2(450.0, 20.0));
      float d = min(d1, d2);
      return smoothstep(hw, -hw, d);
   }

   else if(obstacle==3)
   {
      float f = 1.0;
      float kick = step(0, fract(0.5*f*time)-0.5)*fract(f*time);
      const float r = 100.0 + 10.0*kick;
      float d1 = distance(pos, vec2(512.5))-r;
      float d2 = 512.0-distance(pos, vec2(512.5));
      float d = min(d1, d2);
      return smoothstep(hw, -hw, d);
   }

   else if(obstacle==4)
   {
      vec2 c = vec2(256.0);
      //vec2 p = mod(pos+0.5*c, c)-0.5*c;
      vec2 p = mod(pos, c)-0.5*c;
      float d = sdBox(p, vec2(62.0));
      return smoothstep(hw, -hw, d);
   }
}

void obstacle_n(vec2 pos, inout neighborhood n)
{
   float o = 1.0-get_obstacle(pos);
   n.c *= o;
   n.c2 *= o;

   n.n *= 1.0-get_obstacle(pos+vec2(0,+1));
   n.s *= 1.0-get_obstacle(pos+vec2(0,-1));
   n.e *= 1.0-get_obstacle(pos+vec2(+1,0));
   n.w *= 1.0-get_obstacle(pos+vec2(-1,0));
}

neighborhood get_wrap(ivec2 coord)
{
   neighborhood n;
   n.c = texelFetch(u1, coord, 0);
   n.c2 = texelFetch(u2, coord, 0);
   n.n = texelFetch(u1, wrap_coord(coord+ivec2(0,+1)), 0);
   n.s = texelFetch(u1, wrap_coord(coord+ivec2(0,-1)), 0);
   n.e = texelFetch(u1, wrap_coord(coord+ivec2(+1,0)), 0);
   n.w = texelFetch(u1, wrap_coord(coord+ivec2(-1,0)), 0);
   return n;
}

neighborhood get_clamp(ivec2 coord)
{
   neighborhood n;
   n.c = texelFetch(u1, coord, 0);
   n.c2 = texelFetch(u2, coord, 0);
   n.n = texelFetch(u1, clamp_coord(coord+ivec2(0,+1)), 0);
   n.s = texelFetch(u1, clamp_coord(coord+ivec2(0,-1)), 0);
   n.e = texelFetch(u1, clamp_coord(coord+ivec2(+1,0)), 0);
   n.w = texelFetch(u1, clamp_coord(coord+ivec2(-1,0)), 0);

   return n;
}

neighborhood get_zero(ivec2 coord)
{
   neighborhood n;
   n.c = texelFetch(u1, coord, 0);
   n.c2 = texelFetch(u2, coord, 0);
   n.n = texelFetch(u1, coord+ivec2(0,+1), 0);
   n.s = texelFetch(u1, coord+ivec2(0,-1), 0);
   n.e = texelFetch(u1, coord+ivec2(+1,0), 0);
   n.w = texelFetch(u1, coord+ivec2(-1,0), 0);

   vec2 size = textureSize(u1, 0);
   if(coord.x < 0) n.w = vec4(0.0);
   if(coord.y < 0) n.s = vec4(0.0);
   if(coord.x >= size.x) n.e = vec4(0.0);
   if(coord.y >= size.y) n.n = vec4(0.0);
   
   return n;
}

vec4 wave(neighborhood n)
{
   //damped
   vec4 W = (2.0-4.0*lambda-beta)*n.c + lambda*(n.n+n.s+n.e+n.w) - (1.0-beta)*n.c2;

   //undamped
   //vec4 W = (2.0-4.0*lambda)*n.c + lambda*(n.n+n.s+n.e+n.w) - n.c2;
   return W;
}

vec4 diffusion(neighborhood n)
{
   float D = 0.1;
   float dt = 0.1;
   vec4 L = D*(n.n+n.s+n.e+n.w-4.0*n.c);
   return n.c + dt*L;
}

vec4 reaction_diffusion(neighborhood n)
{
   //float D = 0.1;
   float dt = 0.1;
   const float feed = params[1][0]*0.1;
   const float kill = params[1][1]*0.1;
   const float D = params[0][2];
   const float D0 = params[0][0];
   const float D1 = params[0][1];

   vec4 L = D*(n.n+n.s+n.e+n.w-4.0*n.c);

   vec4 dc = vec4(0.0);
   dc[0] = (D0*L.r - n.c.r*n.c.g*n.c.g + feed*(1.0 - n.c.r));
   dc[1] = (D1*L.g + n.c.r*n.c.g*n.c.g - (feed+kill)*n.c.g);
      
   return n.c + dt*dc;
}

void add_source(inout vec4 fragcolor)
{
   if(source==0)
   {
      return;
   }
   if(source==1)
   {
      const float pi = 3.1415926535;
      float r0 = 39.0;
      float r = 320.0;
      vec2 cen = vec2(512.0);
      float f = 6.0;
      float ang = f*time;
      ang = 2.0*pi/3.0*floor(0.5*ang)+ 0.5*pi;
      
      vec2 pt = vec2(r*cos(1.0*ang)+cen.x, r*sin(1.0*ang)+cen.x);
      if(distance(gl_FragCoord.xy, pt.xy)<r0)
      {
         fragcolor = 0.75*vec4(sin(20.0*time));
      }
   }

   if(source==2)
   {
      const float pi = 3.1415926535;
      float r0 = 39.0;
      float r = 320.0;
      vec2 cen = vec2(512.0);
      float f = 2.0;
      float ang = f*time;
      //ang = floor(ang);
      
      vec2 pt = vec2(0.0*r*cos(1.0*ang)+cen.x, r*sin(1.0*ang)+cen.x);
      if(distance(gl_FragCoord.xy, pt.xy)<r0)
      {
         fragcolor = 0.75*vec4(sin(1.0*time));
      }
   }

   if(source==3)
   {
      const float pi = 3.1415926535;
      float r0 = 39.0;
      float r = 320.0;
      vec2 cen = vec2(512.0);
      float f = 2.0;
      float ang = f*time;
      //ang = floor(ang);
      
      vec2 pt = vec2(r*cos(1.0*ang)+cen.x, r*sin(1.0*ang)+cen.x);
      if(distance(gl_FragCoord.xy, pt.xy)<r0)
      {
         fragcolor = 0.75*vec4(sin(20.0*time));
      }
   }

   if(source==4)
   {
      const float pi = 3.1415926535;
      float r0 = 39.0;
      float r = 320.0;
      vec2 cen = vec2(512.0);
      float f = 6.0;
      float ang = f*time;
      ang = 2.0*pi/4.0*floor(0.5*ang)+ 0.5*pi;
      
      vec2 pt = vec2(r*cos(1.0*ang)+cen.x, r*sin(1.0*ang)+cen.x);
      if(distance(gl_FragCoord.xy, pt.xy)<r0)
      {
         fragcolor = 0.75*vec4(sin(20.0*time));
      }
   }
}

void add_brush(inout vec4 fragcolor)
{
   const float brush_radius = mouse_pos.z;
   vec2 diff = abs(gl_FragCoord.xy - mouse_pos.xy);

   float mouse_dist = 1e16;
   if(brush==0)
   {
      //circle brush
      mouse_dist = length(diff);
   }
   else if(brush==1)
   {
      //diamond brush
      mouse_dist = dot(diff, vec2(1.0));
   }
   else if(brush==2)
   {
      //square brush
      mouse_dist = max(diff.x, diff.y);
   }

   if(mouse_buttons[0]==1 && mouse_dist<=brush_radius)
   {  
      float edge = smoothstep(-0.75*brush_radius, -0.25*brush_radius, mouse_dist-brush_radius);
      //fragcolor = mix(vec4(sin(5.0*time)), fragcolor, edge);
      fragcolor = mix(vec4(1.0), fragcolor, edge);
      //fragcolor += vec4(sin(5.0*time))*edge;
   }
   if(mouse_buttons[1]==1 && mouse_dist<brush_radius)
   {  
      //paint obstacle
      //fragcolor = vec4(1.0);
   }
}

