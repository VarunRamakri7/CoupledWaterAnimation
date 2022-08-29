#pragma once

#include <glm/glm.hpp>
#include <list>

class Module
{
   public:
      Module();
      virtual ~Module();
      virtual void Init() {}
      virtual void Draw() {}
      virtual void DrawGui() {}
      virtual void Compute() {}
      virtual void Animate(float t = -1.0f, float dt = -1.0f) {}
      virtual void Keyboard(int key, int scancode, int action, int mods) {}
      virtual void MouseCursor(glm::vec2 pos) {}
      virtual void MouseButton(int button, int action, int mods, glm::vec2 pos) {}

   static std::list<Module*>& sAllModules();

   static void sInitAll();
   static void sDrawAll();
   static void sDrawGuiAll();
   static void sComputeAll();
   static void sAnimateAll(float t, float dt);
   static void sKeyboardAll(int key, int scancode, int action, int mods);
   static void sMouseCursorAll(glm::vec2 pos);
   static void sMouseButtonAll(int button, int action, int mods, glm::vec2 pos);
};
