#version 430
layout(binding = 0) uniform sampler2D input;
layout(binding = 1) uniform sampler2D bright;
layout(binding = 2) uniform sampler2D h_blur;
layout(binding = 3) uniform sampler2D blur;


layout(location = 0) uniform int pass=1;

in vec2 tex_coord;
out vec4 fragcolor; //the output color for this fragment   

vec4 brightpass(sampler2D tex);
vec4 gauss7_blur_h(sampler2D tex);
vec4 gauss7_blur_v(sampler2D tex);
vec4 bloom();

void main(void)
{   
	if(pass == 0)
   {
		//fragcolor = texelFetch(input, ivec2(gl_FragCoord), 0);
      fragcolor = brightpass(input);

		//fragcolor = textureLod(input, tex_coord, 0);
   }
   if(pass == 1)
   {
		//fragcolor = texelFetch(bright, ivec2(gl_FragCoord), 0);
      fragcolor = gauss7_blur_h(bright);
   }
   if(pass == 2)
   {
		//fragcolor = texelFetch(h_blur, ivec2(gl_FragCoord), 0);
      fragcolor = gauss7_blur_v(h_blur);
   }
   if(pass == 3)
   {
		//fragcolor = texelFetch(blur, ivec2(gl_FragCoord), 0); 
      fragcolor = bloom();
   }
}

vec4 brightpass(sampler2D tex)
{
	ivec2 coord = ivec2(gl_FragCoord);
	vec4 image = texelFetch(input, coord, 0);	
	//vec4 image = textureLod(input, tex_coord, 3);
	image = smoothstep(0.75, 0.9, image);	
	return image;
}

vec4 bloom()
{
	ivec2 coord = ivec2(gl_FragCoord);
	vec4 image = texelFetch(input, coord, 0);	
	vec4 b = texelFetch(blur, coord, 0);
	
	//return b;

	//b = b*vec4(0.1, 0.2, 1.7, 1.0);
	//vec4 final = 0.99*image+12.5*b;
	
	b = b.ggga*vec4(0.5, 0.25, 0.0, 1.0);
	vec4 final = 0.99*image+1.5*b;

	final = pow(final, vec4(1.0/2.2));
	return final;
}

const float gauss7[7] = float[](0.071303,	0.131514,	0.189879,	0.214607,	0.189879,	0.131514,	0.071303);
const float gauss23[23] = float[](0.010230868327997258,
	0.014462601810552199,
	0.01978162627317547,
	0.02617937984475987,
	0.033522694628774365,
	0.04153375650318773,
	0.04979051899757898,
	0.05775309926075923,
	0.06481665267814692,
	0.07038492223923316,
	0.07395279951282308,
	0.07518215984602358,
	0.07395279951282308,
	0.07038492223923316,
	0.06481665267814692,
	0.05775309926075923,
	0.04979051899757898,
	0.04153375650318773,
	0.033522694628774365,
	0.02617937984475987,
	0.01978162627317547,
	0.014462601810552199,
	0.010230868327997258);

vec4 gauss7_blur_h(sampler2D tex)
{
	vec4 color = vec4(0.0);
	for(int i=-11; i<=11; i++)
	{
		ivec2 coord = ivec2(gl_FragCoord)+ivec2(i, 0);
		coord.x = clamp(coord.x, 0, textureSize(tex, 0).x-1);
		color += gauss23[i+11]*texelFetch(tex, coord, 0);
	}
	return color;
}

vec4 gauss7_blur_v(sampler2D tex)
{
	vec4 color = vec4(0.0);

	for(int i=-11; i<=11; i++)
	{
		ivec2 coord = ivec2(gl_FragCoord)+ivec2(0, i);
		coord.y = clamp(coord.y, 0, textureSize(tex, 0).y-1);
		color += gauss23[i+11]*texelFetch(tex, coord, 0);
	}
	return color;		
}


