#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

//layout(binding = 0) uniform sampler2D wave_tex;
layout(binding = 1) uniform samplerCube skybox_tex;
layout(binding = 2) uniform sampler2D fbo_tex;
//layout(binding = 3) uniform sampler2D depth_tex;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV; // Projection x View Matrix
   vec4 eye_w; // Camera eye in world-space
};

in VertexData
{
	vec3 particle_pos;
	vec2 tex_coord;
	float depth;
} inData;

out layout(location = 0) vec4 frag_color; //the output color for this fragment    

const vec3 normal = vec3(0.0f, 1.0f, 0.0f);
const vec4 particle_col = vec4(0.4f, 0.7f, 1.0f, 1.0f); // Light blue
const vec4 foam_col = vec4(1.0f); // White

const vec3 light_col = vec3(1.0f, 0.85f, 0.7f); // Warm light
const vec3 light_pos = vec3(1.0f, 1.0f, 0.0f); // Light position

const float near = 0.1f; // Near plane distance
const float far = 100.0f; // Far plane distance

float LinearizeDepth(float depth);
vec4 reflection();
vec4 refraction();
vec4 lighting();

void main ()
{    
    if(pass == 0)
    {
        // Make circular particles
        float r = length(gl_PointCoord - vec2(0.5f));
        if (r >= 0.5f) discard;
	
	    // Add specular highlight
	    float spec = pow(max(dot(normalize(inData.particle_pos), normalize(light_pos)), 0.0f), 16.0f);
	    float a = exp(-12.0f * r) + 0.1f * exp(-2.0f * r); // Change transparency based on distance from center

        // Calculate lighting and skybox color
        vec4 refraction_color = refraction();
        vec4 reflection_color = reflection();
        vec4 lighting_color = 0.1f * lighting();
    
        vec4 combine = 0.75f * (reflection_color + refraction_color) + lighting_color;

	    // Change particle color depending on height
	    frag_color = mix(combine, foam_col, 2.0f * inData.particle_pos.y);
	    frag_color.rgb += spec; // Add specular highlight
	    frag_color.a = mix(0.1f, a, 2.0f * inData.particle_pos.y);

        //float depth = LinearizeDepth(inData.depth) / far;
        //frag_color = vec4(vec3(depth), 1.0f);
        //ivec2 coord = ivec2(inData.particle_pos.xy);
        //imageStore(depth_tex, coord, vec4(vec3(depth), 1.0f));
    }

    if(pass == 1)
    {
        frag_color = texelFetch(fbo_tex, ivec2(gl_FragCoord), 0);
    }

    if (pass == 2)
    {
        float depth = LinearizeDepth(inData.depth) / far;
        frag_color = vec4(vec3(depth), 1.0f);
    }
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0f - 1.0f; // back to NDC 
    return (2.0f * near * far) / (far + near - z * (far - near));	
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 reflection()
{
    vec3 I = normalize(inData.particle_pos - eye_w.xyz);
    vec3 R = reflect(I, normal);
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 refraction()
{
    float ratio = 1.0f / 1.33f; // Refractive index of Air/Water
    vec3 I = normalize(inData.particle_pos - eye_w.xyz);
    vec3 R = refract(I, normal, ratio);
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/2.1.basic_lighting_diffuse/2.1.basic_lighting.fs
vec4 lighting()
{
    // ambient
    float ambientStrength = 0.8f;
    vec3 ambient = ambientStrength * light_col;
  	
    // diffuse 
    vec3 lightDir = normalize(light_pos - inData.particle_pos);
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * light_col;
    diffuse *= 0.05f;
    
    // specular
    float specularStrength = 10.0f;
    vec3 viewDir = normalize(eye_w.xyz - inData.particle_pos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    vec3 specular = specularStrength * spec * light_col;  
        
    return vec4((ambient + diffuse + specular) * particle_col.xyz, 1.0f);
}