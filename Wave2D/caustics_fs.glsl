#version 430
layout(binding = 0) uniform sampler2D input;

layout(location = 0) uniform int pass=1;

in vec2 tex_coord;
in vec3 p0;
in vec3 p1;
in vec3 r;
in vec3 debug;

out vec4 fragcolor; 

vec4 check(vec2 uv);

void main(void)
{   
	float eps = 1e-16;
	float a0 = length(dFdx(p0))*length(dFdy(p0));
	float a1 = length(dFdx(p1))*length(dFdy(p1))+eps;
	fragcolor = vec4(0.1+0.6*a0/a1)*vec4(0.2, 0.8, 0.7, 0.60);
	//fragcolor = vec4(0.3, 0.95, 0.9, 0.2*a0/a1);
	//fragcolor = vec4(0.2*a0/a1);


	//refraction
	//fragcolor = check(tex_coord+10.0*r.xy/r.z);


	//fragcolor = vec4(0.0);	
	//fragcolor = texture(input, tex_coord);
	//fragcolor = vec4(abs(debug), 1.0);

	//float w = texture(input, tex_coord).r;
	//fragcolor = vec4(vec3(0.5*w+0.5), 1.0);
}

vec4 check(vec2 uv)
{
	uv = fract(10.0*uv)-vec2(0.5);
	return vec4(step(uv.x * uv.y, 0.0));
}