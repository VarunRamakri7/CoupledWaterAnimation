#version 450
layout(binding = 0) uniform sampler2D diffuse_tex;
uniform float time;

in vec2 tex_coord; 

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	vec4 color0 = vec4(0.85f, 0.95f, 1.0f, 1.0f); // Whitish-blue
	vec4 color1 = vec4(0.0f, 0.6f, 0.9f, 1.0f); // Purple
	vec4 v = texture(diffuse_tex, tex_coord);
	v.x = smoothstep(-0.01, 0.01, v.x);
	
	fragcolor = vec4(mix(color1, color0, v.x));
	//fragcolor = vec4(1.0);
}
