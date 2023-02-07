#version 450

layout(local_size_x = 1024) in;

const int UPSWEEP_PHASE = 0;
const int DOWNSWEEP_PHASE = 1;

layout(location=0) uniform int phase = UPSWEEP_PHASE; 
layout(location=1) uniform int stride = 2;
layout(location=2) uniform int n = 2; 

layout (std430, binding = 1) buffer INOUT 
{
	int x[];
};


void main()
{
	int gid = int(gl_GlobalInvocationID.x);
	
	//Stride = 2, 4, 8, 16, ...
	int ka = (stride/2-1) + gid*stride; 
	int kb = ka + stride/2;
	if(kb >= n) return;
	
	if(phase == UPSWEEP_PHASE)
	{
		x[kb] = x[ka] + x[kb];
	}

	if(phase == DOWNSWEEP_PHASE)
	{
		if(stride==n)
		{
			x[n-1] = 0;
		}
		int t = x[ka];
		x[ka] = x[kb];
		x[kb] = t + x[kb];
	}

}


