#version 450
layout(binding=0) uniform sampler2D tex;
uniform float time;
uniform int uMode = 0;

out vec4 fragcolor; //the output color for this fragment    
in vec4 v;
in vec2 tex_coord;

void main(void)
{   
	if(uMode==0)
	{
		float d = length(gl_PointCoord.xy-vec2(0.5));
		if(d>0.5)discard;

		vec2 p = 2.0*gl_PointCoord.xy-vec2(1.0);
		float z = sqrt(abs(0.25-dot(p,p)));
		vec3 n = vec3(p.x, -p.y, z);

		const float var = 0.12;
		float a = exp(-d*d/var);
	
		//fragcolor = vec4(n*a, a);
		fragcolor = vec4(0.5, 0.5, 0.5, smoothstep(0.5, 0.4, d)); //debug
	}
	if(uMode==1)
	{
		float y = 1.0-tex_coord.y;
		vec4 bg1 = vec4(0.6, 0.75, 1.0, 0.0);
		vec4 bg0 = 1.5*vec4(1.0, 0.7, 0.5, 0.0);
		vec4 bg = mix(bg1, bg0, y);

		vec4 p = texture(tex, tex_coord);
		float a = smoothstep(1.0*0.75, 1.0*1.0, p.a);

		vec3 n = normalize(p.xyz*a);
		vec3 l = vec3(0.5, 0.71, +0.21);
		float diff = max(0.0, dot(n, l));
		vec3 v = vec3(0.0, 0.0, 1.0);
		vec3 r = reflect(-l, n);
		float spec = pow(max(0.0, dot(r, v)), 15.0);
		
		//vec4 water = vec4(0.75, 0.75, 1.0, 1.0)*p.a;
		vec4 water = exp(-(vec4(1.0)-vec4(0.1, 0.94, 0.92, 1.0))*sqrt(p.a));

		vec4 final_color = mix(bg*(1.0+0.05*a), water, 0.65*a)*(1.0+0.05*diff+1.0*spec);
		//gamma correction
		final_color = pow(final_color, vec4(1.2));
		fragcolor = final_color;
		//fragcolor.rgb = abs(normalize(p.xyz));
		//fragcolor.rgb = n;
		//fragcolor.rgb = vec3(0.0+0.0*diff+10.0*spec);
		//fragcolor.rgb = r;
		//fragcolor.rgb = vec3(diff + 10.0*spec);
		//fragcolor.rgb = vec3(a);
		//fragcolor = (1.0+0.05*a)*bg;
		//fragcolor = p;

		
	}
}




















