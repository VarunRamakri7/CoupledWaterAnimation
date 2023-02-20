#version 450

layout(local_size_x = 1024) in;

layout(rgba32f, binding = 1) writeonly restrict uniform image1D uOutputImage; //wave at time t
layout(rgba32f, binding = 0) readonly restrict uniform image1D uInputImage; //wave at time t-1


layout (binding = 3) uniform sampler1D uInitImage;

const int MODE_INIT_0 = 0;
const int MODE_INIT_1 = 1;
const int MODE_INIT_FROM_TEXTURE = -1; //no init from texture
const int MODE_ITERATE0 = 2;
const int MODE_ITERATE1 = 3;

const float VIEW_WIDTH = 2.0*4.8;
const float VIEW_HEIGHT = 2.0*4.8;

layout(location=0) uniform int uMode;
layout(location=1) uniform float time;
//wave equation parameters
layout(location = 2) uniform float lambda = 0.001; //alpha*h/k (alpha=const, h=grid spacing, k=time step)
layout(location = 3) uniform float dx = 0.1;
layout(location = 4) uniform float beta = 0.001; //damping
layout(location = 5) uniform vec2 boundary = vec2(0.0);
const float boundary_scale = 0.1;

void InitWave(int coord, int size);
void InitFromImage(int coord);
void IterateWave(int coord, int size);
void Splash(int coord, int size);

struct neighborhood
{
	vec4 c;
	vec4 e;
	vec4 w;
};

neighborhood get_n(int coord);


bool fake_wave = false;
void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	int size = imageSize(uOutputImage);

	if(gid >= size) return;

	if(fake_wave==true)
	{
		float h = 0.1*sin(0.02*gid + 2.1*time);
		imageStore(uOutputImage, gid, vec4(h, 0.0, 0.0, 0.0));
		return;
	}

	switch(uMode)
	{
		case MODE_INIT_0:
			InitWave(gid, size);
		break;

		case MODE_INIT_1:
			Splash(gid, size);
		break;

		case MODE_INIT_FROM_TEXTURE:
			InitFromImage(gid);
		break;

		case MODE_ITERATE0:
		case MODE_ITERATE1:
			IterateWave(gid, size);
		break;
	}
}

void InitFromImage(int coord)
{
	vec4 vout = texelFetch(uInitImage, coord, 0);
	imageStore(uOutputImage, coord, vout);
}

const int REFLECT = 0;
const int FREE = 1;
const int FIXED = 2;
const int BC = FREE;

void EnforceBC(in vec4 c, int coord, int size);
void ReflectBC(in vec4 c, int coord, int size);
void FreeBC(in vec4 c, int coord, int size);
void FixedBC(in vec4 c, int coord, int size);
float hat(float x, float sigma)
{
	float c = x*x/(sigma*sigma);
	return (1-c)*exp(-c/2.0);
}

void Splash(int coord, int size)
{
	vec4 vout = imageLoad(uInputImage, coord);
	float x = float(coord)/float(size-1);
	float cen = 0.5;
	float xc = x-cen;

	float h = -VIEW_HEIGHT*0.1*exp(-xc*xc/0.005);
	//float h = VIEW_HEIGHT*(0.1*hat(xc, 0.10));
	vout.x += h;
	
	//vout.yzw = vec3(0.0);
	vout.y += 0.2*abs(h)*sign(xc);
	imageStore(uOutputImage, coord, vout);
}

void InitWave(int coord, int size)
{
	vec4 vout = vec4(0.0);
	float x = float(coord)/float(size-1);
	float cen = 0.5;
	float xc = x-cen;

	float h_free = VIEW_HEIGHT*0.5;
	float h = VIEW_HEIGHT*0.1*exp(-xc*xc/0.005);
	//float h = VIEW_HEIGHT*0.1*hat(xc, 0.10);
	vout.x = h_free+h;
	vout.y = 0.5*abs(h)*sign(xc);

	imageStore(uOutputImage, coord, vout);
}

void IterateWave(int coord, int size)
{
	neighborhood n = get_n(coord);

	//vec4 layout: vec4(h, uh, hm, uhm)
	const int H = 0;
	const int UH = 1;
	const int HM = 2;
	const int UHM = 3;

	const float G = 9.8;
	vec4 w = n.c;
	if(uMode==MODE_ITERATE0)
	{
		float hm = (n.c[H] + n.e[H])/2.0 - lambda/2.0*(n.e[UH]-n.c[UH])/dx;

		float uhm = (n.c[UH] + n.e[UH])/2.0 - lambda/2.0*(n.e[UH]*n.e[UH]/n.e[H] + 0.5*G*n.e[H]*n.e[H]
														 -n.c[UH]*n.c[UH]/n.c[H] - 0.5*G*n.c[H]*n.c[H])/dx;
		w[HM] = hm;
		w[UHM] = uhm;
		EnforceBC(w, coord, size);
		imageStore(uOutputImage, coord, w); // no BC needed for half values
	}
	if(uMode==MODE_ITERATE1)
	{
		float h = n.c[H] - lambda*(n.c[UHM]-n.w[UHM])/dx;

		float uh = n.c[UH] - lambda*(n.c[UHM]*n.c[UHM]/n.c[HM] + 0.5*G*n.c[HM]*n.c[HM]
									-n.w[UHM]*n.w[UHM]/n.w[HM] - 0.5*G*n.w[HM]*n.w[HM])/dx;

		w[H] = h;
		w[UH] = uh;
		EnforceBC(w, coord, size);
	}
}

void EnforceBC(in vec4 c, int coord, int size)
{
	if(BC==FREE) FreeBC(c, coord, size);
	else if(BC==REFLECT) ReflectBC(c, coord, size);
	else if(BC==FIXED) FixedBC(c, coord, size);
}

void FreeBC(in vec4 c, int coord, int size)
{
	if(coord==0 || coord == size-1)
	{
		return; //neighbors will write boundary values
	}

	imageStore(uOutputImage, coord, c);

	//write boundary values
	if(coord==1)
	{
		imageStore(uOutputImage, 0, c);	
	}
	if(coord==size-2)
	{
		imageStore(uOutputImage, size-1, c);	
	}
}

void ReflectBC(in vec4 c, int coord, int size)
{
	if(coord==0 || coord == size-1)
	{
		return; //neighbors will write
	}

	imageStore(uOutputImage, coord, c);

	c[1] = -c[1]; //relect only velocity
	if(coord==1 && BC==REFLECT)
	{
		imageStore(uOutputImage, 0, c);	
	}
	if(coord==size-2 && BC==REFLECT)
	{
		imageStore(uOutputImage, size-1, c);	
	}	
}

void FixedBC(in vec4 c, int coord, int size)
{
	const vec4 left = vec4(boundary[0]*boundary_scale, 0.0, 0.0, 0.0);
	const vec4 right = vec4(boundary[1]*boundary_scale, 0.0, 0.0, 0.0);

	//write boundary values
	if(coord==0)
	{
		imageStore(uOutputImage, coord, left);
		return;
	}
	if(coord==size-1)
	{
		imageStore(uOutputImage, coord, right);
		return;
	}

	imageStore(uOutputImage, coord, c);

}

neighborhood get_n(int coord)
{
	neighborhood n;
	n.c = imageLoad(uInputImage, coord);
	n.e = imageLoad(uInputImage, coord+1);
	n.w = imageLoad(uInputImage, coord-1);

   return n;
}
