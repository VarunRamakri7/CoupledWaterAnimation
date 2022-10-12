#pragma once
#include <string>
#include <vector>
#include <GL/glew.h>

class ImagePicker
{
   public:
      struct Item
      {
         std::string mFilename;
         GLuint mTexture;
         float mAspect;

         Item(std::string filename, GLuint texture=-1);
      };

      bool DrawGui(const char* label);
      void AddItem(std::string filename);
      Item* GetLastPickedItem() {return pLastPickedItem;}

   private:
      std::vector<Item> mItems;
      int mIconHeight = 200;
      Item* pLastPickedItem = nullptr;

   friend ImagePicker* CreatePickerFromFilenames(const std::vector<std::string> names);
};
