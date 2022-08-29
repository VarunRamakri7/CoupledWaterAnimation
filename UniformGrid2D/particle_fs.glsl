#version 430

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 M;
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};


in float age;

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
   float d = length(gl_PointCoord.xy-vec2(0.5));
   float a = smoothstep(0.5, 0.49, d);
   if(a<=0.0) discard;
   fragcolor = vec4(0.4, 0.175, 0.1, 2.25*a);  //hot colors
   //fragcolor = vec4(0.1, 0.2, 0.4, 0.15*a);    //cool colors
   fragcolor = pow(fragcolor, vec4(1.15));
   //fragcolor = vec4(1.0);
}

