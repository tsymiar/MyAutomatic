#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengl.h>

int SDL_GL_init();
void SDL_GL_SetAs2DMode(int w, int h);
void SDL_GL_Leave2DMode();
GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord);
