#version 450

layout(binding = 0) uniform sampler2D diffuse_tex;
layout(location = 1) uniform float time;

in VertexData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
   vec3 color;
} inData;

out vec4 fragcolor; //the output color for this fragment  

const vec3 wave_col = vec3(1.0f, 0.0f, 0.0f); // Red color

void main(void)
{   
	//fragcolor = vec4(1.0, 0.0, 0.0, 1.0);
	//fragcolor = vec4(inData.tex_coord, 0.0, 1.0); //debug view tex coords
	//fragcolor = vec4(abs(nw), 1.0); //debug view normals

	fragcolor = vec4(wave_col, 1.0f);
}

/*in vec2 tex_coord; 

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	vec4 color0 = vec4(1.0f, 0.95f, 0.85, 1.0f); // Whitish-Red
	vec4 color1 = vec4(0.9f, 0.6f, 0.0f, 1.0f); // Purple
	vec4 v = texture(diffuse_tex, tex_coord);
	v.x = smoothstep(-0.01, 0.01, v.x);
	
	fragcolor = vec4(mix(color1, color0, v.x));
	//fragcolor = vec4(1.0);
}*/