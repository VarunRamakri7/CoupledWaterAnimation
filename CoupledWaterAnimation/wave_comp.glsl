#version 450

#define WAVE_RES 512
#define PARTICLE_RADIUS 0.005f

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba32f, binding = 0) readonly restrict uniform image2D uInputImage0; //wave at time t-1
layout(rgba32f, binding = 1) readonly restrict uniform image2D uInputImage1; //wave at time t-2
layout(rgba32f, binding = 2) writeonly restrict uniform image2D uOutputImage; //wave at time t

layout (binding = 3) uniform sampler2D uInitImage;

layout(location = 1) uniform float time;
layout(location = 3) uniform int uMode;

const int MODE_INIT = 0;
const int MODE_INIT_FROM_TEXTURE = 1;
const int MODE_EVOLVE = 2;
const int MODE_TEST = 10;

struct Wave
{
    vec4 pos;
    vec4 tex_coords; // XY - UV, ZW - Grid coordinate
    vec4 normals;
};

layout(std430, binding = 1) buffer WAVE
{
    Wave waves[];
};

layout(std140, binding = 2) uniform BoundaryUniform
{
    vec4 upper; // Upper bounds of particle area
    vec4 lower; // Lower bounds of particle area
};

layout(std140, binding = 3) uniform WaveUniforms
{
	vec4 attributes; // Lambda, Attenuation, Beta
};

const float dt = 0.0001f; // Time step

void InitWave(ivec2 coord);
void InitFromImage(ivec2 coord);
void EvolveWave(ivec2 coord, ivec2 size);

struct neighborhood
{
	vec4 c1;
	vec4 c0;
	vec4 n0;
	vec4 s0;
	vec4 e0;
	vec4 w0;
};

neighborhood get_clamp(ivec2 coord);

void main()
{
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(uOutputImage);

	if(any(greaterThanEqual(gid.xy, size))) return;

	switch(uMode)
	{
		case MODE_INIT:
			InitWave(gid);
			//InitFromImage(gid);
		break;

		case MODE_INIT_FROM_TEXTURE:
			InitFromImage(gid);
		break;

		case MODE_EVOLVE:
			EvolveWave(gid, size);
		break;
		case MODE_TEST:
			break;
	}
}

void InitFromImage(ivec2 coord)
{
	vec4 vout = texelFetch(uInitImage, coord, 0);
	imageStore(uOutputImage, coord, vout);
}

void InitWave(ivec2 coord)
{
	vec4 vout = vec4(0.0);
	ivec2 size = imageSize(uOutputImage);

	ivec2 cen0;
	ivec2 cen1;
	ivec2 cen2;

	if(attributes.w == 1.0f)
	{
		// Splash
		cen0 = ivec2(0.25f * size);
		cen1 = ivec2(0.75f * size);
	}
	else
	{
		// Wave
		cen0 = ivec2(0.25f * size.x, size.y);
		cen2 = ivec2(0.5f * size.x, size.y);
		cen1 = ivec2(0.75f * size.x, size.y);
	}

	float d = min(distance(coord, cen0), distance(coord, cen1));
	if(attributes.w == 0.0f) //Add third center for wave
	{
		d = min(d, distance(coord, cen2));
	}

	vout.x = (attributes.w == 0.0f ? 1.0f : 0.5f) * smoothstep(5.0f, 0.0f, d);
	imageStore(uOutputImage, coord, vout);
}

void EvolveWave(ivec2 coord, ivec2 size)
{
	neighborhood n = get_clamp(coord);
	vec4 w = (2.0 - 4.0 * attributes[0] - attributes[2]) * n.c0 + attributes[0] * (n.n0 + n.s0 + n.e0 + n.w0) - (1.0 - attributes[2]) * n.c1;
	w *= attributes[1];

    imageStore(uOutputImage, coord, w);
}

ivec2 clamp_coord(ivec2 coord)
{
   ivec2 size = imageSize(uOutputImage);
   return ivec2(clamp(coord, ivec2(0), size-ivec2(1)));
}

neighborhood get_clamp(ivec2 coord)
{
	neighborhood n;
	n.c1 = imageLoad(uInputImage1, coord);
	n.c0 = imageLoad(uInputImage0, coord);
	n.n0 = imageLoad(uInputImage0, clamp_coord(coord+ivec2(0,+1)));
	n.s0 = imageLoad(uInputImage0, clamp_coord(coord+ivec2(0,-1)));
	n.e0 = imageLoad(uInputImage0, clamp_coord(coord+ivec2(+1,0)));
	n.w0 = imageLoad(uInputImage0, clamp_coord(coord+ivec2(-1,0)));

   return n;
}