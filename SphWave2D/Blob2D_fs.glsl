#version 450
layout(binding=0) uniform sampler2D tex;
layout(binding=1) uniform sampler2D datatex;
uniform float time;
uniform int uMode = 0;

layout (location=0) out vec4 fragcolor; //the output color for this fragment    
layout (location=1) out vec4 data;
in vec4 pv;
in vec4 pa;
in vec2 tex_coord;

const float VIEW_WIDTH = 2.0*4.8;//3*WIDTH*PARTICLE_DIAM;
const float VIEW_HEIGHT = 2.0*4.8;//3*HEIGHT*PARTICLE_DIAM;

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
		//a = 1.0;
	
		fragcolor = vec4(n*a, a);
		//fragcolor = vec4(0.5, 0.5, 0.5, smoothstep(0.5, 0.4, d)); //debug
		data = vec4(pa*a);
	}
	if(uMode==1)
	{
		float y = 1.0-tex_coord.y;
		const vec4 bg1 = vec4(0.6, 0.75, 1.0, 0.0);
		const vec4 bg0 = 1.5*vec4(1.0, 0.7, 0.5, 0.0);
		vec4 bg = mix(bg1, bg0, y);
		//fragcolor = texture(tex, tex_coord);

		vec4 p = texture(tex, tex_coord);
		vec4 pdata = texture(datatex, tex_coord);
		float a = smoothstep(0.5*0.75, 0.99*1.0, p.a);

		vec3 n = normalize(p.xyz*a);
		vec3 l = vec3(0.5, 0.71, +0.21);
		float diff = max(0.0, dot(n, l));
		vec3 v = vec3(0.0, 0.0, 1.0);
		vec3 r = reflect(-l, n);
		float spec = pow(max(0.0, dot(r, v)), 15.0);
		
		//vec4 water = vec4(0.75, 0.75, 1.0, 1.0)*p.a;
		float rho = smoothstep(3000000.0, 4500000.0, pdata.w);
		const vec4 w0 = vec4(0.1, 0.94, 0.92, 1.0);
		const vec4 w1 = vec4(0.1, 0.90, 0.82, 1.0);
		vec4 w = vec4(1.0)-mix(w0, w1, rho);
		vec4 water = exp(-w*sqrt(p.a));

		

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
		//fragcolor = water;
		//fragcolor = vec4(smoothstep(10000000.0, 20000000.0, pdata.w)); //pressure
		//fragcolor = vec4(smoothstep(1500.0, 2400.0, length(pdata.xy)));

		//draw sphere
		if(false)
		{
			vec2 pos = tex_coord*vec2(VIEW_WIDTH, VIEW_HEIGHT);
			const float r_path = 3.0;
			const vec2 cen = vec2(0.5*VIEW_WIDTH+r_path*cos(time), 0.5*VIEW_HEIGHT+r_path*sin(time));
			const float r = 1.0;
			float R = length(pos-cen);
			
			const vec4 sun0 = 0.85*vec4(1.0, 0.5, 0.15, 1.0);
			const vec4 sun1 = 0.8*vec4(1.0, 1.0, 0.85, 1.0);
			if(R<r)
			{
				fragcolor = mix(sun0, sun1, R*R);
			}
			if(R<1.3*r)
			{
				fragcolor += 0.2*sun0*vec4(smoothstep(0.3, 0.0, abs(R-r)));
			}
		}
	}
}




















