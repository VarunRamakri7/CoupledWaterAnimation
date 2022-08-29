#version 430
layout(binding = 0) uniform sampler2D down;
layout(binding = 1) uniform sampler2D up;
layout(binding = 2) uniform sampler2D input;

layout(location = 0) uniform int pass = 0;
layout(location = 1) uniform int read_level = 0;


//pass 0: downsample
//pass 1: upsample and add

in vec2 tex_coord;
out vec4 fragcolor; //the output color for this fragment   

const int max_level = 7;

float luminance(vec3 c)
{
   return dot(vec3(0.2126, 0.7152, 0.0722), c.rgb);
}

void main(void)
{   
   ivec2 texSize = textureSize(down, read_level);
   vec2 texelSize = 1.0f/vec2(texSize);
   ivec2 icoord = ivec2(gl_FragCoord.xy);
      
   if(pass == -1) //brightpass
   {
      vec4 image = texelFetch(input, icoord, 0);	
      float lum = luminance(image.rgb);
      fragcolor = image*smoothstep(1.0, 1.1, lum);	
   }

	if(pass == 0)//downsample
   {
      vec4 outcolor = vec4(0.0);
/*
		outcolor += 0.25*textureLod(down, tex_coord - 0.0*texelSize + vec2(0.0, 0.0)*texelSize, read_level);
      outcolor += 0.25*textureLod(down, tex_coord - 0.0*texelSize + vec2(1.0, 0.0)*texelSize, read_level);
      outcolor += 0.25*textureLod(down, tex_coord - 0.0*texelSize + vec2(0.0, 1.0)*texelSize, read_level);
      outcolor += 0.25*textureLod(down, tex_coord - 0.0*texelSize + vec2(1.0, 1.0)*texelSize, read_level);
      fragcolor = outcolor;
//*/

//*
      vec4 a = textureLod(down, tex_coord - vec2(0.5, 0.5)*texelSize, read_level);
      vec4 b = textureLod(down, tex_coord - vec2(1.5, 1.5)*texelSize, read_level);
      vec4 c = textureLod(down, tex_coord - vec2(1.5, -0.5)*texelSize, read_level);
      vec4 d = textureLod(down, tex_coord - vec2(-0.5, 1.5)*texelSize, read_level);
      vec4 e = textureLod(down, tex_coord - vec2(-0.5, -0.5)*texelSize, read_level);
//*
      float wa = 1.0/(1.0+luminance(a.rgb));
      float wb = 1.0/(1.0+luminance(b.rgb));
      float wc = 1.0/(1.0+luminance(c.rgb));
      float wd = 1.0/(1.0+luminance(d.rgb));
      float we = 1.0/(1.0+luminance(e.rgb));
//*/
/*
      float wa = 0.2;
      float wb = 0.2;
      float wc = 0.2;
      float wd = 0.2;
      float we = 0.2;
*/
      fragcolor = (wa*a + wb*b + wc*c + wd*d + we*e)/(wa+wb+wc+wd+we);
//*/
      
   }
   else if(pass == 1) //upsample
   {
      vec4 bloom = vec4(0.0);
      if(read_level<max_level)
      {
         /*
         bloom += 0.25*textureLod(up, tex_coord - 0.5*texelSize + vec2(0.0, 0.0)*texelSize, read_level+1);
         bloom += 0.25*textureLod(up, tex_coord - 0.5*texelSize + vec2(1.0, 0.0)*texelSize, read_level+1);
         bloom += 0.25*textureLod(up, tex_coord - 0.5*texelSize + vec2(0.0, 1.0)*texelSize, read_level+1);
         bloom += 0.25*textureLod(up, tex_coord - 0.5*texelSize + vec2(1.0, 1.0)*texelSize, read_level+1);
         */

         bloom += textureLod(up, tex_coord + vec2(0.5, 0.5)*texelSize, read_level+1);
         bloom += textureLod(up, tex_coord + vec2(1.5, 1.5)*texelSize, read_level+1);
         bloom += textureLod(up, tex_coord + vec2(-0.5, 1.5)*texelSize, read_level+1);
         bloom += textureLod(up, tex_coord + vec2(1.5, -0.5)*texelSize, read_level+1);
         bloom += textureLod(up, tex_coord + vec2(-0.5, -0.5)*texelSize, read_level+1);
         bloom *= 0.2;
      }
      float w[10] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
      vec4 down_img = texelFetch(down, icoord, read_level);
      fragcolor = down_img + w[read_level]*bloom;
   }
   else if(pass == 2)
   {
      vec4 image = texelFetch(input, icoord, 0);
      vec4 bloom = texelFetch(up, icoord, 0);
      fragcolor = image+bloom/2.0;

      //fragcolor = pow(fragcolor, vec4(1.0/2.2));
   }
}

