#version 450

layout(binding = 0) uniform sampler2D wave_tex;
layout(binding = 1) uniform samplerCube skybox_tex;
layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;
   vec4 eye_w; // Camera eye in world-space
};

in VertexData
{
   vec2 tex_coord;
   vec3 pw;
   vec3 nw;
   vec3 color;
} inData;

out vec4 fragcolor;

const vec4 wave_col = vec4(0.7f, 0.9f, 1.0f, 1.0f); // Wave col
const vec4 color0 = vec4(0.6f, 0.8f, 1.0f, 1.0f); // Light blue

const vec3 light_col = vec3(1.0f, 0.85f, 0.7f); // Warm light
const vec3 light_pos = vec3(0.0f, 1.0f, 0.0f); // Light position
const vec4 base_col = vec4(0.6f, 0.8f, 1.0f, 1.0f); // Base wave col

vec4 reflection();
vec4 refraction();
vec4 lighting();

void main(void)
{
	vec4 v = texture(wave_tex, inData.tex_coord);
	v.x = smoothstep(-0.01, 0.01, v.x);
	
	//fragcolor = vec4(mix(wave_col, color0, v.x)); // Color wave depending on height

    //fragcolor = 0.5f * (refraction() + reflection()); // Color wave with refraction and reflection
    //fragcolor = 5.0f * lighting();// Color wave with lighting

    vec4 refraction_color = refraction();
    vec4 reflection_color = reflection();
    vec4 lighting_color = lighting();

    vec4 final_color = mix(base_col, 0.5f * (refraction() + reflection()) + lighting(), v.x); // Mix all attributes for final color
    //vec4 final_color = 0.5f * (refraction_color + reflection_color) + 0.75f * lighting_color + base_col;

    fragcolor = final_color;
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 reflection()
{
    vec3 I = normalize(inData.pw - eye_w.xyz);
    vec3 R = reflect(I, normalize(inData.nw));
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 refraction()
{
    float ratio = 1.0f / 1.33f; // Refractive index of Air/Water
    vec3 I = normalize(inData.pw - eye_w.xyz);
    vec3 R = refract(I, normalize(inData.nw), ratio);
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/2.1.basic_lighting_diffuse/2.1.basic_lighting.fs
vec4 lighting()
{
    // ambient
    float ambientStrength = 0.2f;
    vec3 ambient = ambientStrength * light_col;
  	
    // diffuse 
    vec3 norm = normalize(inData.nw);
    vec3 lightDir = normalize(light_pos - inData.pw);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = diff * light_col;
    
    // specular
    float specularStrength = 2.0f;
    vec3 viewDir = normalize(eye_w.xyz - inData.pw);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    vec3 specular = specularStrength * spec * light_col;  
        
    return vec4((ambient + diffuse + specular) * color0.xyz, 1.0f);
}