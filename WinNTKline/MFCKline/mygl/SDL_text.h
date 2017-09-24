#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_opengl.h>

int SDL_GL_init();
void SDL_GL_Enter2DMode(int w, int h);
void SDL_GL_Leave2DMode();
GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord);
