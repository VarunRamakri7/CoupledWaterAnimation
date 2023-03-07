#include <windows.h>

#include "..\imgui-master\imgui.h"
#include "..\imgui-master\backends\imgui_impl_glfw.h"
#include "..\imgui-master\backends\imgui_impl_opengl3.h"
#include "..\implot-master\implot.h"
#include "..\implot-master\implot_internal.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "VideoMux.h"      //Functions for saving videos
#include "Surf.h"
#include "StencilImage2DTripleBuffered.h"
#include "DebugCallback.h"

#define RESTART_INDEX 65535
#define WAVE_RES 512

#define NUM_PARTICLES 20480
#define PARTICLE_RADIUS 0.005f
#define WORK_GROUP_SIZE 1024
#define PART_WORK_GROUPS 20 // Ceiling of particle count divided by work group size
#define MAX_WAVE_WORK_GROUPS 32 // Work group size for wave compute shader

const int init_window_width = 720;
const int init_window_height = 720;
const char* const window_title = "Coupled Water Animation";

// Vertex and Fragment Shaders
static const std::string particle_vs("particle_vs.glsl");
static const std::string particle_fs("particle_fs.glsl");
static const std::string wave_vs("wave_vs.glsl");
static const std::string wave_fs("wave_fs.glsl");
static const std::string skybox_vs("skybox_vs.glsl");
static const std::string skybox_fs("skybox_fs.glsl");

// Compute Shaders
static const std::string rho_pres_com_shader("rho_pres_comp.glsl");
static const std::string force_comp_shader("force_comp.glsl");
static const std::string integrate_comp_shader("integrate_comp.glsl");

// Shader programs
GLuint particle_shader_program = -1;
GLuint wave_shader_program = -1;
GLuint skybox_shader_program = -1;
GLuint compute_programs[3] = { -1, -1, -1 };

GLuint particle_position_vao = -1;
GLuint particles_ssbo = -1;

indexed_surf_vao strip_surf;
ComputeShader waveCS("wave_comp.glsl");
StencilImage2DTripleBuffered wave2d;

// Skybox Textures
std::vector<std::string> faces =
{ 
    "skybox-textures/posx.png", // Right
    "skybox-textures/negx.png", // Left
    "skybox-textures/posy.png", // Top
    "skybox-textures/negy.png", // Bottom
    "skybox-textures/posz.png", // Back
    "skybox-textures/negz.png" // Front
};
const float skyboxVertices[] = {
    // positions          
    -50.0f,  50.0f, -50.0f,
    -50.0f, -50.0f, -50.0f,
    50.0f, -50.0f, -50.0f,
    50.0f, -50.0f, -50.0f,
    50.0f,  50.0f, -50.0f,
    -50.0f,  50.0f, -50.0f,

    -50.0f, -50.0f,  50.0f,
    -50.0f, -50.0f, -50.0f,
    -50.0f,  50.0f, -50.0f,
    -50.0f,  50.0f, -50.0f,
    -50.0f,  50.0f,  50.0f,
    -50.0f, -50.0f,  50.0f,
    
    50.0f, -50.0f, -50.0f,
    50.0f, -50.0f,  50.0f,
    50.0f,  50.0f,  50.0f,
    50.0f,  50.0f,  50.0f,
    50.0f,  50.0f, -50.0f,
    50.0f, -50.0f, -50.0f,

     -50.0f, -50.0f,  50.0f,
    -50.0f,  50.0f,  50.0f,
    50.0f,  50.0f,  50.0f,
    50.0f,  50.0f,  50.0f,
    50.0f, -50.0f,  50.0f,
    -50.0f, -50.0f,  50.0f,

    -50.0f,  50.0f, -50.0f,
    50.0f,  50.0f, -50.0f,
    50.0f,  50.0f,  50.0f,
    50.0f,  50.0f,  50.0f,
    -50.0f,  50.0f,  50.0f,
    -50.0f,  50.0f, -50.0f,

    -50.0f, -50.0f, -50.0f,
    -50.0f, -50.0f,  50.0f,
    50.0f, -50.0f, -50.0f,
    50.0f, -50.0f, -50.0f,
    -50.0f, -50.0f,  50.0f,
    50.0f, -50.0f,  50.0f
};
GLuint skybox_vao;
GLuint skybox_vbo;
GLuint skybox_tex = -1;

GLuint init_wave_tex = -1;

// Perspective view
glm::vec3 eye_persp = glm::vec3(7.0f, 4.0f, 0.0f);
glm::vec3 center_persp = glm::vec3(0.0f, -1.0f, 0.0f);

// Orthographic view
glm::vec3 eye_ortho = glm::vec3(-12.0f, -1.0f, 8.0f);
glm::vec3 center_ortho = glm::vec3(-1.0f, -1.0f, 0.0f);

float angle = 0.75f;
float particle_scale = 5.0f;
float wave_scale = 0.047f;
float aspect = 1.0f;
bool recording = false;
bool simulate = false;

bool drawSurface = true;
bool drawParticles = true;
bool isOrthoView = false;

struct Particle
{
    glm::vec4 pos;
    glm::vec4 vel;
    glm::vec4 force;
    glm::vec4 extras; // 0 - rho, 1 - pressure, 2 - age
};

//This structure mirrors the uniform block declared in the shader
struct SceneUniforms
{
    glm::mat4 PV; //camera projection * view matrix
    glm::vec4 eye_w; //world-space eye position
} SceneData;

struct ConstantsUniform
{
    float mass = 0.02f; // Particle Mass
    float smoothing_coeff = 2.0f; // Smoothing length coefficient for neighborhood
    float visc = 3000.0f; // Fluid viscosity
    float resting_rho = 1000.0f; // Resting density
}ConstantsData;

struct BoundaryUniform
{
    glm::vec4 upper = glm::vec4(0.48f, 1.0f, 0.48f, 500.0f); // XYZ - Upper bounds, W - Foam Threshold
    glm::vec4 lower = glm::vec4(0.0f, -0.02f, 0.0f, 50.0f); // XYZ - Lower bounds, W - Density coefficient
}BoundaryData;

struct WaveUniforms
{
    glm::vec4 attributes = glm::vec4(0.01f, 0.995f, 0.001f, 1.0f); // Lambda, Attenuation, Beta, Wave type
} WaveData;

GLuint scene_ubo = -1;
GLuint constants_ubo = -1;
GLuint boundary_ubo = -1;
GLuint wave_ubo = -1;
namespace UboBinding
{
    int scene = 0;
    int constants = 1;
    int boundary = 2;
    int wave = 3;
}

//Locations for the uniforms which are not in uniform blocks
namespace UniformLocs
{
    int M = 0; // model matrix
    int time = 1;
    int pass = 2;
    int mode = 3; // Unused
}

void draw_gui(GLFWwindow* window)
{
    //Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //Draw Gui
    ImGui::Begin("Debug window");
    if (ImGui::Button("Quit"))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    const int filename_len = 256;
    static char video_filename[filename_len] = "capture.mp4";

    ImGui::InputText("Video filename", video_filename, filename_len);
    if (recording == false)
    {
        if (ImGui::Button("Start Recording"))
        {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            recording = true;
            start_encoding(video_filename, w, h); //Uses ffmpeg
        }

    }
    else
    {
        if (ImGui::Button("Stop Recording"))
        {
            recording = false;
            finish_encoding(); //Uses ffmpeg
        }
    }

    ImGui::SliderFloat("View angle", &angle, -glm::pi<float>(), +glm::pi<float>());
    ImGui::SliderFloat("Particle Scale", &particle_scale, 0.0001f, 20.0f);
    ImGui::SliderFloat("Wave Scale", &wave_scale, 0.0001f, 1.0f);
    ImGui::Checkbox("Orthographic View", &isOrthoView);
    if (!isOrthoView)
    {
        // Change perspective camera
        ImGui::SliderFloat3("Camera Eye", &eye_persp[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("Camera Center", &center_persp[0], -10.0f, 10.0f);

        //WaveData.attributes[1] = 0.995f; // Set attenuation for splash
        WaveData.attributes[3] = 1.0f; // Set wave mode as splash
        angle = 0.75f;
    }
    else
    {
        // Change orthographic camera
        ImGui::SliderFloat3("Camera Eye", &eye_ortho[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("Camera Center", &center_ortho[0], -10.0f, 10.0f);

        //WaveData.attributes[1] = 0.998f; // Set attenuation for wave
        WaveData.attributes[3] = 0.0f; // Set wave mode as wave
        angle = 0.65f;
    }

    ImGui::Checkbox("Draw Wave Surface", &drawSurface);
    ImGui::Checkbox("Draw Particles", &drawParticles);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("Constants Window");
    ImGui::SliderFloat("Mass", &ConstantsData.mass, 0.01f, 10.0f);
    ImGui::SliderFloat("Smoothing", &ConstantsData.smoothing_coeff, 2.0f, 10.0f);
    ImGui::SliderFloat("Viscosity", &ConstantsData.visc, 1000.0f, 5000.0f);
    ImGui::SliderFloat("Resting Density", &ConstantsData.resting_rho, 1000.0f, 5000.0f);
    ImGui::SliderFloat3("Upper Bounds", &BoundaryData.upper[0], 0.001f, 1.0f);
    ImGui::SliderFloat3("Lower Bounds", &BoundaryData.lower[0], -1.0f, -0.001f);
    ImGui::SliderFloat("Lamba", &WaveData.attributes[0], 0.01f, 0.09f);
    ImGui::SliderFloat("Attenuation", &WaveData.attributes[1], 0.9f, 1.0f);
    ImGui::SliderFloat("Beta", &WaveData.attributes[2], 0.001f, 0.01f);
    ImGui::End();

    Module::sDrawGuiAll();

    //End ImGui Frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
    //Clear the screen to the color previously specified in the glClearColor(...) call.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 V;
    glm::mat4 P;
    if (isOrthoView)
    {
        // Orthographic View
        SceneData.eye_w = glm::vec4(eye_ortho, 1.0f);
        V = glm::lookAt(glm::vec3(SceneData.eye_w), center_ortho, glm::vec3(0.0f, 1.0f, 0.0f));
        P = glm::ortho(0.0f, 5.0f, 0.0f, 5.0f, 0.001f, 100.0f);
    }
    else
    {
        // Set perspective view
        SceneData.eye_w = glm::vec4(eye_persp, 1.0f);
        V = glm::lookAt(glm::vec3(SceneData.eye_w), center_persp, glm::vec3(0.0f, 1.0f, 0.0f));
        P = glm::perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 50.0f);
    }
    SceneData.PV = P * V;

    //Set uniforms
    glm::mat4 M = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(particle_scale));
    glProgramUniformMatrix4fv(particle_shader_program, UniformLocs::M, 1, false, glm::value_ptr(M)); // Set particle Model Matrix

    M = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(wave_scale));
    glProgramUniformMatrix4fv(wave_shader_program, UniformLocs::M, 1, false, glm::value_ptr(M)); // Set Wave Model Matrix

    glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo); //Bind the OpenGL UBO before we update the data.
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneUniforms), &SceneData); //Upload the new uniform values.

    glBindBuffer(GL_UNIFORM_BUFFER, constants_ubo); // Bind the OpenGL UBO before we update the data.
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ConstantsUniform), &ConstantsData); // Upload the new uniform values.

    glBindBuffer(GL_UNIFORM_BUFFER, boundary_ubo); // Bind the OpenGL UBO before we update the data.
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(BoundaryUniform), &BoundaryData); // Upload the new uniform values.

    glBindBuffer(GL_UNIFORM_BUFFER, wave_ubo); // Bind the OpenGL UBO before we update the data.
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(WaveUniforms), &WaveData); // Upload the new uniform values.

    glBindBuffer(GL_UNIFORM_BUFFER, 0); //unbind the ubo

    // Draw skybox
    glUseProgram(skybox_shader_program); // Use wave shader program
    glDepthMask(GL_FALSE);
    glBindVertexArray(skybox_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    // Draw Particles
    if (drawParticles)
    {
        glUseProgram(particle_shader_program);
        glBindVertexArray(particle_position_vao);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES); // Draw particles
    }

    // Draw wave surface
    if (drawSurface)
    {
        glUseProgram(wave_shader_program); // Use wave shader program

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);

        wave2d.GetReadImage(0).BindTextureUnit();
        glm::ivec3 size = wave2d.GetReadImage(0).GetSize();
        glBindVertexArray(strip_surf.vao);
        strip_surf.Draw();
    }
    
    glBindVertexArray(0); // Unbind VAO

    if (recording == true)
    {
        glFinish();
        glReadBuffer(GL_BACK);
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        read_frame_to_encode(&rgb, &pixels, w, h);
        encode_frame(rgb);
    }

    draw_gui(window);

    /* Swap front and back buffers */
    glfwSwapBuffers(window);
}

void idle()
{
    static float time_sec = 0.0f;
    time_sec += 1.0f / 60.0f;

    //Pass time_sec value to the shaders
    glProgramUniform1f(particle_shader_program, UniformLocs::time, time_sec);
    glProgramUniform1f(wave_shader_program, UniformLocs::time, time_sec);

    // Dispatch compute shaders
    if (simulate)
    {
        glUseProgram(compute_programs[0]); // Use density and pressure calculation program
        glDispatchCompute(PART_WORK_GROUPS, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glUseProgram(compute_programs[1]); // Use force calculation program
        glDispatchCompute(PART_WORK_GROUPS, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glUseProgram(compute_programs[2]); // Use integration calculation program
        glDispatchCompute(PART_WORK_GROUPS, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
        glBindTextureUnit(3, init_wave_tex);
        Module::sComputeAll();
    }
}

void reload_shader()
{
    GLuint new_particle_shader = InitShader(particle_vs.c_str(), particle_fs.c_str());
    GLuint new_wave_shader = InitShader(wave_vs.c_str(), wave_fs.c_str());
    GLuint new_skybox_shader = InitShader(skybox_vs.c_str(), skybox_fs.c_str());

    // Load compute shaders
    GLuint compute_shader_handle = InitShader(rho_pres_com_shader.c_str());
    if (compute_shader_handle != -1)
    {
        compute_programs[0] = compute_shader_handle;
    }

    compute_shader_handle = InitShader(force_comp_shader.c_str());
    if (compute_shader_handle != -1)
    {
        compute_programs[1] = compute_shader_handle;
    }

    compute_shader_handle = InitShader(integrate_comp_shader.c_str());
    if (compute_shader_handle != -1)
    {
        compute_programs[2] = compute_shader_handle;
    }

    /*compute_shader_handle = InitShader(wave_comp_shader.c_str());
    if (compute_shader_handle != -1)
    {
        compute_programs[3] = compute_shader_handle;
    }*/
    waveCS.Init();

    // Check particle shader program
    if (new_particle_shader == -1) // loading failed
    {
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
    }
    else
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        if (particle_shader_program != -1)
        {
            glDeleteProgram(particle_shader_program);
        }
        particle_shader_program = new_particle_shader;

        glLinkProgram(particle_shader_program);
    }

    // Check wave shader program
    if (new_wave_shader == -1)
    {
        glClearColor(0.0f, 1.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
    }
    else
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        if (wave_shader_program != -1)
        {
            glDeleteProgram(wave_shader_program);
        }
        wave_shader_program = new_wave_shader;

        glLinkProgram(wave_shader_program);
    }

    // Check skybox shader program
    if (new_skybox_shader == -1)
    {
        glClearColor(0.0f, 1.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
    }
    else
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        if (skybox_shader_program != -1)
        {
            glDeleteProgram(skybox_shader_program);
        }
        skybox_shader_program = new_skybox_shader;

        glLinkProgram(skybox_shader_program);
    }
}

void init_particles();

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //std::cout << "key : " << key << ", " << char(key) << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << std::endl;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case 'r':
        case 'R':
            init_particles();
            reload_shader();
            wave2d.Reinit();
            break;

        case 'p':
        case 'P':
            simulate = !simulate;
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }

    Module::sKeyboardAll(key, scancode, action, mods);
}

//This function gets called when the mouse moves over the window.
void mouse_cursor(GLFWwindow* window, double x, double y)
{
    //std::cout << "cursor pos: " << x << ", " << y << std::endl;
    Module::sMouseCursorAll(glm::vec2(x, y));
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    //std::cout << "button : "<< button << ", action: " << action << ", mods: " << mods << std::endl;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    Module::sMouseButtonAll(button, action, mods, glm::vec2(x, y));
}

void resize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height); //Set viewport to cover entire framebuffer
    aspect = float(width) / float(height); // Set aspect ratio
}

/// <summary>
/// Make positions for a cube grid
/// </summary>
/// <returns>Vector of positions for the grid</returns>
std::vector<glm::vec4> make_cube()
{
    std::vector<glm::vec4> positions;

    const float spacing = ConstantsData.smoothing_coeff * 0.85f * PARTICLE_RADIUS;
    //const float spacing = (BoundaryData.upper.x - BoundaryData.lower.x) / 25;

    // 50x4x50 cuboid of particles above the wave surface
    const float mid = 0.0f; // (BoundaryData.upper.x + BoundaryData.lower.x) / 4.0f;
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            for (int k = 0; k < 64; k++)
            {
                float x = mid + i * spacing;
                float z = mid + k * spacing;
                positions.push_back(glm::vec4(x, j * spacing, z, 1.0f));
            }
        }
    }
    //std::cout << "Position count: " << positions.size() << std::endl;

    return positions;
}

/// <summary>
/// Initialize the SSBO with a cube of particles
/// </summary>
void init_particles()
{
    // Initialize particle data
    std::vector<Particle> particles(NUM_PARTICLES);
    std::vector<glm::vec4> grid_positions = make_cube(); // Get grid positions
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        particles[i].pos = grid_positions[i];
        particles[i].vel = glm::vec4(0.0f); // // No initial velocity
        particles[i].force = glm::vec4(0.0f); // No initial force
        particles[i].extras = glm::vec4(ConstantsData.resting_rho, 0.0f, 500.0f, 50.0f); // 0 - rho, 1 - pressure, 2 - foam threshold, 3 - density coefficient
    }
    //std::cout << "Particles count: " << particles.size() << std::endl;

    // Generate and bind shader storage buffer
    glGenBuffers(1, &particles_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particles_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * NUM_PARTICLES, particles.data(), GL_STREAM_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particles_ssbo);

    // Generate and bind VAO for particle positions
    glGenVertexArrays(1, &particle_position_vao);
    glBindVertexArray(particle_position_vao);

    glBindBuffer(GL_ARRAY_BUFFER, particles_ssbo);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr); // Bind buffer containing particle positions to VAO
    glEnableVertexAttribArray(0); // Enable attribute with location = 0 (vertex position) for VAO

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind SSBO
    glBindVertexArray(0); // Unbind VAO
}

void init_skybox()
{
    glGenVertexArrays(1, &skybox_vao);
    glGenBuffers(1, &skybox_vbo);
    glBindVertexArray(skybox_vao);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

#define BUFFER_OFFSET(offset) ((GLvoid*) (offset))

//Initialize OpenGL state. This function only gets called once.
void initOpenGL()
{
    glewInit();
    RegisterDebugCallback();

    int max_work_groups = -1;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_work_groups);

    //Print out information about the OpenGL version supported by the graphics driver.	
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Max work group invocations: " << max_work_groups << std::endl;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    init_skybox();
    skybox_tex = LoadCubemap(faces);
    init_wave_tex = LoadTexture("init-textures/wake.png");
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

    init_particles();

    reload_shader();

    waveCS.SetMaxWorkGroupSize(glm::ivec3(MAX_WAVE_WORK_GROUPS, MAX_WAVE_WORK_GROUPS, 1));
    wave2d.SetShader(waveCS);

    strip_surf = create_indexed_surf_strip_vao(WAVE_RES);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glPointSize(8.0f);

    Module::sInitAll();

    //Create and initialize uniform buffers

    // For SceneUniforms
    glGenBuffers(1, &scene_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneUniforms), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::scene, scene_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    // For ConstantsUniform
    glGenBuffers(1, &constants_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, constants_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ConstantsUniform), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::constants, constants_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    // For BoundaryUniform
    glGenBuffers(1, &boundary_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, boundary_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(BoundaryUniform), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::boundary, boundary_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    // For WaveUniforms
    glGenBuffers(1, &wave_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, wave_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(WaveUniforms), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::wave, wave_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

//C++ programs start executing in the main() function.
int main(int argc, char** argv)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(init_window_width, init_window_height, window_title, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    //Register callback functions with glfw. 
    glfwSetKeyCallback(window, keyboard);
    glfwSetCursorPosCallback(window, mouse_cursor);
    glfwSetMouseButtonCallback(window, mouse_button);
    glfwSetFramebufferSizeCallback(window, resize);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    initOpenGL();

    //Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        idle();
        display(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}