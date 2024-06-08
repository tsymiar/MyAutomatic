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

struct CopyRect {
    SDL_Rect src;
    SDL_Rect dst;
};

struct TextCfg {
    const char* font;
    int style = TTF_STYLE_NORMAL;
    SDL_Colour color;
    CopyRect rect;
};

int SDL_GL_init();
int SDL_GL_loadImage(const char* filename, CopyRect rect);
void SDL_GL_showText(const char* text, TextCfg cfg);
void SDL_GL_SetAs2DMode(int w, int h);
void SDL_GL_Leave2DMode();
GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* coord);
void SDL_GL_quit();
static void SDL_GL_SwapBuffers() {}

#endif // _WIN32
