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

layout(std140, binding = 3) uniform WaveUniforms
{
	vec4 attributes; // Lambda, Attenuation, Beta
	vec4 mesh_ws_pos; // World-space mesh position
};

float theta = 0.0f;
int anim_mode = -1;

const float dt = 0.0001f; // Time step

void InitWave(ivec2 coord);
void InitFromImage(ivec2 coord);
void EvolveWave(ivec2 coord, ivec2 size);
bool CoordOnCircle(ivec2 coord, ivec2 size);
bool CoordOnLine(ivec2 coord, ivec2 size);

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
	vec4 vout = texelFetch(uInitImage, coord * ivec2(2, 1), 0);
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

		anim_mode = 0;
	}
	else if (attributes.w == 0.0f)
	{
		// Wave
		cen0 = ivec2(0.25f * size.x, size.y);
		cen2 = ivec2(0.5f * size.x, size.y);
		cen1 = ivec2(0.75f * size.x, size.y);

		anim_mode = 1;
	}
	else
	{
		// Boat wake
		cen0 = ivec2(0.5f * size.x, 0.1f * size.y);
		cen2 = cen0;//ivec2(0.5f * size.x, 0.2f * size.y);
		cen1 = cen0;//ivec2(0.5f * size.x, 0.3f * size.y);

		anim_mode = 2;
	}

	float d = min(distance(coord, cen0), distance(coord, cen1));
	if(attributes.w != 1.0f) //Add third center for wave, and wake
	{
		d = min(d, distance(coord, cen2));
	}

	float peak = 0.5f;
	switch(anim_mode)
	{
		case 0: // Splash
				peak = 0.5f;
				break;
		case 1: // Wave
			peak = 1.0f;
			break;
		case 2: // Boat wake
			peak = 0.1f;
			break;
	}

	vout.x = peak * smoothstep(5.0f, 0.0f, d);
	imageStore(uOutputImage, coord, vout);
}

bool CoordOnCircle(ivec2 coord, ivec2 size)
{
	vec2 tex_coord = vec2(coord) / vec2(size);
	vec2 center = vec2(0.5f);

	float radius = length(tex_coord - center);
	float circle_radius = 0.5f;
	float tolerance = 0.01f;

	if(abs(radius - circle_radius) < tolerance)
	{
		return true;
	}

	return false;
}

bool CoordOnLine(ivec2 coord, ivec2 size)
{
	int mid_x = size.x / 2;

	if(coord.x == mid_x)
	{
		return true;
	}

	return false;
}

void EvolveWave(ivec2 coord, ivec2 size)
{
	neighborhood n = get_clamp(coord);
	vec4 w = (2.0f - 4.0f * attributes[0] - attributes[2]) * n.c0 + attributes[0] * (n.n0 + n.s0 + n.e0 + n.w0) - (1.0f - attributes[2]) * n.c1;
	w *= attributes[1];

	if(w.r > 0.0001f && attributes.w > 0.0f && attributes.w < 1.0f)
	{
		//if(CoordOnCircle(coord, size)) // Check if this coord lies on a circle
		if(CoordOnLine(coord, size)) // Check if this coord lies on a line
		{
			w.r += 0.001f; // Increase if this coord lies on the circle
		}
	}

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