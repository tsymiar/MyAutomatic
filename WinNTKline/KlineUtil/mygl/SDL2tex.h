#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

struct stCopyRect {
    SDL_Rect src;
    SDL_Rect dst;
};

struct TextCfg {
    const char* font;
    int style = TTF_STYLE_NORMAL;
    SDL_Colour color;
    struct stCopyRect rect;
};

int SDL_GL_init();
int SDL_GL_loadImage(const char* filename, struct stCopyRect rect);
void SDL_GL_showText(const char* text, struct TextCfg cfg);
void SDL_GL_SetAs2DMode(int w, int h);
void SDL_GL_Leave2DMode();
GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* coord);
void SDL_GL_quit();
static void SDL_GL_SwapBuffers() { }
