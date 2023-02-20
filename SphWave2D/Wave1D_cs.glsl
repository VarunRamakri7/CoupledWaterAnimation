#version 450

layout(local_size_x = 1024) in;

layout(rgba32f, binding = 2) writeonly restrict uniform image1D uOutputImage; //wave at time t
layout(rgba32f, binding = 0) readonly restrict uniform image1D uInputImage0; //wave at time t-1
layout(rgba32f, binding = 1) readonly restrict uniform image1D uInputImage1; //wave at time t-2

layout (binding = 3) uniform sampler1D uInitImage;

const int MODE_INIT_0 = 0;
const int MODE_INIT_1 = 1;
const int MODE_INIT_FROM_TEXTURE = -1; //no init from texture
const int MODE_ITERATE = 2;

layout(location=0) uniform int uMode;
layout(location=1) uniform float time;
//wave equation parameters
layout(location = 2) uniform float lambda = 0.01; //alpha*h/k (alpha=const, h=grid spacing, k=time step)
layout(location = 3) uniform float atten = 0.9995; //attenuation
layout(location = 4) uniform float beta = 0.001; //damping
layout(location = 5) uniform vec2 boundary = vec2(0.0);
const float boundary_scale = 0.1;

void InitWave(int coord, int size);
void InitFromImage(int coord);
void IterateWave(int coord, int size);

struct neighborhood
{
	vec4 c1;
	vec4 c0;
	vec4 e0;
	vec4 w0;
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
		case MODE_INIT_1:
			InitWave(gid, size);
		break;

		case MODE_INIT_FROM_TEXTURE:
			InitFromImage(gid);
		break;

		case MODE_ITERATE:
			IterateWave(gid, size);
		break;
	}
}

void InitFromImage(int coord)
{
	vec4 vout = texelFetch(uInitImage, coord, 0);
	imageStore(uOutputImage, coord, vout);
}

void ComputeVelocityAcceleration(inout vec4 c, neighborhood n)
{
	float v = (c.x-n.c1.x)/2.0;
	float a = (c.x - 2.0*n.c0.x + n.c1.x);
	c.y = v;
	c.z = a;
}

const int REFLECT = 0;
const int FREE = 1;
const int FIXED = 2;
const int BC = FREE;

void EnforceBC(in vec4 c, int coord, int size);
void ReflectBC(in vec4 c, int coord, int size);
void FreeBC(in vec4 c, int coord, int size);
void FixedBC(in vec4 c, int coord, int size);

void InitWave(int coord, int size)
{
	vec4 vout = vec4(0.0);
	int cen = int(0.25*size)+1*uMode;
	int x = (cen-coord);

	float d0 = 0.0;
	if(BC==FIXED) d0 = mix(boundary[0]*boundary_scale, boundary[1]*boundary_scale, float(coord)/float(size-1));
	float h = d0 + 0.1*exp(-x*x/5000.0);// - 0.1*exp(-x*x/1000.0);
	vout.x = h; //position
	imageStore(uOutputImage, coord, vout);
	
	if(uMode == MODE_INIT_1)
	{
		neighborhood n = get_n(coord);
		vec4 w = n.c0 - 0.5*lambda*(n.e0-2.0*n.c0+n.w0);
		
		ComputeVelocityAcceleration(w, n);
		EnforceBC(w, coord, size);
		//imageStore(uOutputImage, coord, w);
	}
	
}

void IterateWave(int coord, int size)
{
	neighborhood n = get_n(coord);

	//vec4 w = (2.0-2.0*lambda)*n.c0 + lambda*(n.e0+n.w0) - n.c1;
	vec4 w = (2.0-2.0*lambda-beta)*n.c0 + lambda*(n.e0+n.w0) - (1.0-beta)*n.c1;
	w = atten*w;

	ComputeVelocityAcceleration(w, n);
	EnforceBC(w, coord, size);
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
		return; //neighbors will write
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

	if(coord==1 && BC==REFLECT)
	{
		imageStore(uOutputImage, 0, -c);	
	}
	if(coord==size-2 && BC==REFLECT)
	{
		imageStore(uOutputImage, size-1, -c);	
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
	n.c1 = imageLoad(uInputImage1, coord);
	n.c0 = imageLoad(uInputImage0, coord);
	n.e0 = imageLoad(uInputImage0, coord+1);
	n.w0 = imageLoad(uInputImage0, coord-1);

   return n;
}
