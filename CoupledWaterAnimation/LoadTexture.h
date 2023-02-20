#ifndef __LOADTEXTURE_H__
#define __LOADTEXTURE_H__

#include <string>
#include <vector>
#include <windows.h>
#include "GL/glew.h"
#include "GL/gl.h"

GLuint LoadTexture(const std::string& fname);
GLuint LoadCubemap(const std::vector<std::string>& faces);

#endif