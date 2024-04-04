#pragma once
#ifdef _WIN32
int SDL_GL_init() { return 0; }
int SDL_GetRevisionNumber() { return 0; }
void SDL_GL_quit() {}
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

int SDL_GL_init();
int SDL_GL_loadImage(const char* filename);
void SDL_GL_SetAs2DMode(int w, int h);
void SDL_GL_Leave2DMode();
GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* coord);
void SDL_GL_quit();

#endif // _WIN32
