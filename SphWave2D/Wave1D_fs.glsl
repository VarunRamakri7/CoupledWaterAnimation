#version 450

layout(binding = 0) uniform sampler1D wave_tex;
uniform float time;

out vec4 fragcolor; //the output color for this fragment    
in vec2 tex_coord; 

const float VIEW_WIDTH = 2.0*4.8;
const float VIEW_HEIGHT = 2.0*4.8;

void main(void)
{   
	vec4 wave = texture(wave_tex, tex_coord.x);

	float h = wave[0];
	float y = VIEW_HEIGHT*tex_coord.y;

	float eps = abs(dFdx(y));
	//float eps = 0.0;
	float w = smoothstep(-2.0*eps, 2.0*eps, h-y);
	fragcolor = vec4(w);

	const vec4 c0 = vec4(1.0, 0.5, 0.75, 1.0);
	const vec4 c1 = vec4(0.5, 1.0, 0.75, 1.0);
	vec4 v_color = mix(c0, c1, smoothstep(-0.2, 0.2, wave[1]/wave[0]));
	fragcolor = vec4(w)*v_color;
	
}




















