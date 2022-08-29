#include "Module.h"

//std::list<Module*> Module::sAllModules; //no, construct on first use instead

Module::Module()
{
   sAllModules().push_back(this);
}

Module::~Module()
{
   sAllModules().remove(this);
}

std::list<Module*>& Module::sAllModules()
{
   static std::list<Module*> all_modules;
   return all_modules;
}

void Module::sInitAll()
{
   for(Module* p: sAllModules())
   {
      p->Init();
   }
}

void Module::sDrawAll()
{
   for(Module* p: sAllModules())
   {
      p->Draw();
   }
}

void Module::sDrawGuiAll()
{
   for(Module* p: sAllModules())
   {
      p->DrawGui();
   }
}

void Module::sComputeAll()
{
   for(Module* p: sAllModules())
   {
      p->Compute();
   }
}

void Module::sAnimateAll(float t, float dt)
{
   for(Module* p: sAllModules())
   {
      p->Animate(t, dt);
   }
}

void Module::sKeyboardAll(int key, int scancode, int action, int mods)
{
   for(Module* p: sAllModules())
   {
      p->Keyboard(key, scancode, action, mods);
   }
}

void Module::sMouseCursorAll(glm::vec2 pos)
{
   for(Module* p: sAllModules())
   {
      p->MouseCursor(pos);
   }
}

void Module::sMouseButtonAll(int button, int action, int mods, glm::vec2 pos)
{
   for(Module* p: sAllModules())
   {
      p->MouseButton(button, action, mods, pos);
   }
}
