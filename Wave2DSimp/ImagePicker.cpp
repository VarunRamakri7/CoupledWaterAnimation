#include "ImagePicker.h"
#include "LoadTexture.h"
#include "imgui.h"
#include <glm/glm.hpp>

ImagePicker::Item::Item(std::string filename, GLuint texture)
{
   mFilename = filename;
   if (texture != -1)
   {
      mTexture = texture;
   }
   else
   {
      mTexture = LoadTexture(mFilename);
   }
   
   int w, h;
   glGetTextureLevelParameteriv(mTexture, 0, GL_TEXTURE_WIDTH, &w);
   glGetTextureLevelParameteriv(mTexture, 0, GL_TEXTURE_HEIGHT, &h);
   mAspect = float(w)/float(h);

}

void ImagePicker::AddItem(std::string filename)
{
   Item item(filename);
   mItems.push_back(item);
}

bool ImagePicker::DrawGui(const char* label)
{
   if (ImGui::Button(label))
   {
      ImGui::OpenPopup("ImagePicker");
   }
   bool popup_open = true;
   if (ImGui::BeginPopupModal("ImagePicker", &popup_open))
   {
      for (Item item : mItems)
      {
         ImVec2 icon_size(item.mAspect * mIconHeight, mIconHeight);
         if (ImGui::ImageButton((void*)item.mTexture, icon_size))
         {
            pLastPickedItem = &item;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
         }
         ImGui::SameLine();
         if (ImGui::Button(item.mFilename.c_str()))
         {
            pLastPickedItem = &item;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
         }
      }
      ImGui::EndPopup();
   }
   
   return false;
}


ImagePicker* CreatePickerFromFilenames(const std::vector<std::string> names)
{
   ImagePicker* pPicker = new ImagePicker();
   for (std::string name : names)
   {
      pPicker->AddItem(name);
   }
   return pPicker;
}