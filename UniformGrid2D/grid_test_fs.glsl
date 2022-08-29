#version 430

layout(location=0) uniform vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

/*
in VertexData
{
   
} inData;   
*/
out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
   fragcolor = color;
}

