#include <windows.h>

#include "Main.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "StencilImage2DTripleBuffered.h"
#include "Shader.h"
#include "AttriblessRendering.h"
#include "DebugCallback.h"
#include "StencilBuffer.h"
#include "BlitFbo.h"
#include "StencilImage2D.h"

RenderFbo render_fbo;
BlitFbo blit_fbo;
glm::ivec2 window_size(1024, 1024);

struct Particle
{
   glm::vec4 pos;
   glm::vec4 vel;
   glm::vec4 force;
};

int num_particles = 32*128;

Shader sph_shader("Sph2D_vs.glsl", "Sph2D_fs.glsl");
Shader blob_shader("Blob2D_vs.glsl", "Blob2D_fs.glsl");
Shader* pShader = &sph_shader;

//ComputeShader SphCS("SphCerrno2D_cs.glsl");
//ComputeShader SphCS("SphMuller2D_cs.glsl");
//ComputeShader SphCS("SphKoschier2D_cs.glsl");
//ComputeShader SphCS("SphKoschier2D_2_cs.glsl");
ComputeShader SphCS("SphWaveKoschier2D_grid_cs.glsl");
ComputeShader* pComputeShader = &SphCS;

//SphMuller sph2d;
SphUgrid sph2d;

Shader WaveShader("Wave1D_vs.glsl", "Wave1D_fs.glsl");
ComputeShader WaveCS("Wave1D_cs.glsl");
ComputeShader ShallowCS("Shallow1D_cs.glsl");
ImageStencil wave1d;

void InitWaveEquation()
{
   WaveCS.Init();
   wave1d.SetShader(WaveCS);
   wave1d.SetNumBuffers(3);//triple buffered
   wave1d.SetSubsteps(10);
   wave1d.SetGridSize(glm::ivec3(1024, 1, 1));
   wave1d.GetWriteImage().SetFilter(glm::ivec2(GL_LINEAR));
   for (int i = 0; i < wave1d.GetNumReadImages(); i++)
   {
      wave1d.GetReadImage(i).SetFilter(glm::ivec2(GL_LINEAR));
   }
}

void InitShallowWaterEquation()
{
   WaveCS.Init();
   wave1d.SetShader(ShallowCS);
   wave1d.SetNumBuffers(2);//double buffered
   //wave1d.SetSubsteps(10);

   //Two phase update for Lax Wendroff
   wave1d.MODE_ITERATE_FIRST = 2;
   wave1d.MODE_ITERATE_LAST = 3;

   wave1d.SetGridSize(glm::ivec3(128, 1, 1));
   wave1d.GetWriteImage().SetFilter(glm::ivec2(GL_LINEAR));
   wave1d.GetWriteImage().SetWrap(glm::ivec3(GL_CLAMP_TO_EDGE));
   for (int i = 0; i < wave1d.GetNumReadImages(); i++)
   {
      wave1d.GetReadImage(i).SetFilter(glm::ivec2(GL_LINEAR));
      wave1d.GetReadImage(i).SetWrap(glm::ivec3(GL_CLAMP_TO_EDGE));
   }

}

bool enable_gui = true;
void draw_gui(GLFWwindow* window)
{
   if(enable_gui==false) return;
   //Begin ImGui Frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   //Draw Gui
   ImGui::Begin(PROJECT_NAME);
      if (ImGui::Button("Quit"))                          
      {
         glfwSetWindowShouldClose(window, GLFW_TRUE);
      }  

      static bool mute_debug = false;
      if (ImGui::Checkbox("Mute Debug Msg", &mute_debug))
      {
         glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, !mute_debug);
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
   ImGui::End();

   if (ImGui::Button("Splash"))
   {
      wave1d.ComputeFunc(1);
      //wave1d.ComputeFunc(1);
      ///SphUgrid.
   }

 
   Module::sDrawGuiAll();

   static bool show_test = false;
   if(show_test)
   {
      ImGui::ShowDemoWindow(&show_test);
   }

   //End ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
   //Clear the screen to the color previously specified in the glClearColor(...) call.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //render wave
   if(1)
   {
      WaveShader.UseProgram();
      wave1d.GetReadImage(0).BindTextureUnit();
      glDepthMask(GL_FALSE);
      draw_attribless_quad();
      glDepthMask(GL_TRUE);
   }

   //Draw blobs
   if(0)
   {
      glClearColor(0.0, 0.0, 0.0, 0.0);
      render_fbo.PreRender();
      pShader->UseProgram();
      pShader->SetMode(0);
      sph2d.GetReadBuffer().BindBufferBase();
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glDepthMask(GL_FALSE);
      glDisable(GL_DEPTH_TEST);

      if (pShader == &blob_shader)
      {
         draw_attribless_particles(num_particles);
      }

      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
      render_fbo.PostRender();

      pShader->SetMode(1);
      glBindTextureUnit(0, render_fbo.mOutputTextures[0]);
      glBindTextureUnit(1, render_fbo.mOutputTextures[1]);
      draw_attribless_quad();
   
      //blit_fbo.Blit();

      glViewport(0, 0, window_size.x, window_size.y);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDrawBuffer(GL_BACK);
   }   


   if(pShader==&sph_shader)
   {
      pShader->UseProgram();
      sph2d.GetReadBuffer().BindBufferBase();
      glDepthMask(GL_FALSE);
      draw_attribless_particles(num_particles);
      glDepthMask(GL_TRUE);
   }


   draw_gui(window);

   /* Swap front and back buffers */
   glfwSwapBuffers(window);
}

void idle()
{
   float time_sec = static_cast<float>(glfwGetTime());

   //Pass time_sec value to the shaders
   int time_loc = pShader->GetUniformLocation("time");
   if (time_loc != -1)
   {
      glProgramUniform1f(pShader->GetShader(), time_loc, time_sec);
   }

   time_loc = pComputeShader->GetUniformLocation("time");
   if (time_loc != -1)
   {
      glProgramUniform1f(pComputeShader->GetShader(), time_loc, time_sec);
   }

   time_loc = WaveCS.GetUniformLocation("time");
   glProgramUniform1f(WaveCS.GetShader(), time_loc, time_sec);

   time_loc = ShallowCS.GetUniformLocation("time");
   glProgramUniform1f(ShallowCS.GetShader(), time_loc, time_sec);
   

   Module::sComputeAll();
   
   SphCS.UseProgram();
   wave1d.GetReadImage(0).BindTextureUnit();
}

//Initialize OpenGL state. This function only gets called once.
void initOpenGL()
{
   glewInit();
   RegisterDebugCallback();


   //Print out information about the OpenGL version supported by the graphics driver.	
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
   int max_work_group_size = -1;
   glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_work_group_size);
   std::cout << "Max work group size: " << max_work_group_size << std::endl;

   glEnable(GL_DEPTH_TEST);

   //Enable gl_PointCoord in shader
   glEnable(GL_POINT_SPRITE);
   //Allow setting point size in fragment shader
   glEnable(GL_PROGRAM_POINT_SIZE);

   sph_shader.Init();
   blob_shader.Init();
   
   bind_attribless_vao();

   //SphCS.SetMaxWorkGroupSize(glm::ivec3(1024, 1, 1));
   SphCS.Init();
   int num_loc = SphCS.GetUniformLocation("num_particles");
   glProgramUniform1i(SphCS.GetShader(), num_loc, num_particles);

   sph2d.SetSubsteps(2);
   sph2d.SetShader(SphCS);
   sph2d.mElementSize = sizeof(Particle);
   sph2d.mNumElements = num_particles;

   WaveShader.Init();
   //InitWaveEquation();
   InitShallowWaterEquation();

   Module::sInitAll();

   render_fbo.Init(window_size, 2);

   blit_fbo.Init(window_size);
   blit_fbo.SetInputTexture(render_fbo.mOutputTextures[0]);
}

void reload_shader()
{
   bool success = pShader->Init();
   
   if (success == false) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
      DebugBreak();
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);
   }

   SphCS.Init();
   int num_loc = SphCS.GetUniformLocation("num_particles");
   glProgramUniform1i(SphCS.GetShader(), num_loc, num_particles);

   WaveShader.Init();
   
}

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action == GLFW_PRESS)
   {
      switch (key)
      {
      case 'r':
      case 'R':
         reload_shader();
         break;

      case GLFW_KEY_F1:
         enable_gui = !enable_gui;
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
   Module::sMouseCursorAll(glm::vec2(x,y));
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
   double x, y;
   glfwGetCursorPos(window, &x, &y);
   Module::sMouseButtonAll(button, action, mods, glm::vec2(x, y));
}

void resize(GLFWwindow* window, int width, int height)
{
   window_size = glm::ivec2(width, height);
   glViewport(0, 0, width, height);
}

//C++ programs start executing in the main() function.
int main(void)
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
   window = glfwCreateWindow(window_size.x, window_size.y, PROJECT_NAME, NULL, NULL);
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