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

Shader wave_mesh_shader("WaveMesh_vs.glsl", "WaveMesh_fs.glsl");
Shader* pShader = &wave_mesh_shader;

ComputeShader WaveCS("Wave2D_cs.glsl");
StencilImage2DTripleBuffered wave2d;

struct ComputeShaderUniforms
{
   //Compute shader
   float Lambda = 0.01f;
   float Atten = 0.9995f;
   float Beta = 0.001f;
   
} WaveUniforms;

struct ComputeShaderUniformLocs
{
   //Compute shader
   int LambdaLoc = 1;
   int AttenLoc = 2;
   int BetaLoc = 3;
} WaveLocs;

void draw_gui(GLFWwindow* window)
{
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

      if (ImGui::SliderFloat("Lambda", &WaveUniforms.Lambda, 0.001f, 0.05f))
      {
         glProgramUniform1f(WaveCS.GetShader(), WaveLocs.LambdaLoc, WaveUniforms.Lambda);
      }
      if (ImGui::SliderFloat("Atten", &WaveUniforms.Atten, 0.990f, 1.0f, "%.4f"))
      {
         glProgramUniform1f(WaveCS.GetShader(), WaveLocs.AttenLoc, WaveUniforms.Atten);
      }
      if (ImGui::SliderFloat("Beta", &WaveUniforms.Beta, 0.0f, 0.05f))
      {
         glProgramUniform1f(WaveCS.GetShader(), WaveLocs.BetaLoc, WaveUniforms.Beta);
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
   ImGui::End();

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
   
   pShader->UseProgram();
   wave2d.GetReadImage(0).BindTextureUnit();

   if (pShader == &wave_mesh_shader)
   {
      glm::mat4 M = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(1.0f));
      glm::mat4 V = glm::lookAt(glm::vec3(0.0f, -3.5f, 1.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      glm::mat4 P = glm::perspective(40.0f, 1.0f, 0.1f, 100.0f);
      glm::mat4 PVM = P * V * M;

      //Set the value of the variable at a specific location
      const int PVM_loc = 0;
      glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glm::ivec3 size = wave2d.GetReadImage(0).GetSize();
      draw_attribless_triangle_grid(size.x, size.y);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

   Module::sComputeAll();
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

   wave_mesh_shader.Init();
   
   bind_attribless_vao();

   WaveCS.SetMaxWorkGroupSize(glm::ivec3(32, 32, 1));
   WaveCS.Init();

   wave2d.SetShader(WaveCS);

   Module::sInitAll();
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
      glClearColor(0.15f, 0.15f, 0.15f, 0.0f);
   }

   WaveCS.Init();
   
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
   window = glfwCreateWindow(700, 700, PROJECT_NAME, NULL, NULL);
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