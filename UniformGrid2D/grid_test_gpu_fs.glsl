#version 430


in VertexData
{
   vec4 color;
} inData;   

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
   fragcolor = inData.color;
}

