#version 450
layout(binding = 0) uniform sampler1D wave_tex;
const float VIEW_WIDTH = 2.0*4.8;//3*WIDTH*PARTICLE_DIAM;
const float VIEW_HEIGHT = 2.0*4.8;//3*HEIGHT*PARTICLE_DIAM;

uniform float time;

out vec4 fragcolor; //the output color for this fragment    
in vec4 v;
in vec4 pos;

void main(void)
{   
	//if(pos.w==0.0) discard;
	float d = length(gl_PointCoord.xy-vec2(0.5));
	float a = smoothstep(0.5, 0.49, d);
	if(a<=0.0) discard;


	vec2 p = 2.0*gl_PointCoord.xy-vec2(1.0);
	float z = sqrt(1.0-dot(p,p));
	vec3 n = vec3(p, z);
	const vec3 l = vec3(0.707, -0.707, 0.707);
	float diff = max(0.0, dot(n,l))+0.05;
	fragcolor = vec4(diff);

	//color based on wave velocity
	vec2 tex_coord = 0.5*pos.xy+vec2(0.5);
	vec4 wave = texture(wave_tex, tex_coord.x);
	float h = wave[0];

	const vec4 c0 = vec4(1.0, 0.5, 0.75, 1.0);
	const vec4 c1 = vec4(0.5, 1.0, 0.75, 1.0);
	const vec4 c2 = vec4(0.5);
	vec4 v_color = mix(c0, c1, smoothstep(-0.2, 0.2, wave[1]));

	if(tex_coord.y*VIEW_HEIGHT > h) v_color = c2;
	v_color *= vec4(0.75, 0.75, 1.0, 1.0);
	fragcolor = diff*v_color;
}




















