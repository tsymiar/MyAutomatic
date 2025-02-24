#include "SDL2tex.h"

#if SDL_MAJOR_VERSION >= 2
static SDL_Window* g_window = NULL;
static SDL_Renderer* g_render = NULL;
static SDL_Surface* g_surface = NULL;
#endif

const int W = 640;
const int H = 480;

int SDL_GL_init()
{
    //Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        SDL_Log("Can't init sdl, %s", SDL_GetError());
        return -1;
    }
#if SDL_MAJOR_VERSION >= 2
    g_window = SDL_CreateWindow("SDL SHOW", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_OPENGL);
    if (g_window == NULL) {
        SDL_Log("Can't creat window, %s", SDL_GetError());
        return -2;
    }
    g_render = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (g_render == NULL) {
        SDL_Log("Can't creat render, %s", SDL_GetError());
        return -3;
    }
    SDL_SetRenderDrawColor(g_render, 255, 255, 255, 255);
    SDL_RenderClear(g_render);
#endif
    if (TTF_Init() == -1) {
        SDL_Log("Can't init TTF, %s", SDL_GetError());
        return -4;
    }
    return 0;
}

int SDL_GL_loadImage(const char* filename, stCopyRect rect)
{
#if SDL_MAJOR_VERSION >= 2
    g_surface = SDL_LoadBMP(filename);
    if (g_surface == NULL) {
        SDL_Log("Can't creat surface, %s", SDL_GetError());
        return -1;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(g_render, g_surface);
    if (texture == NULL) {
        SDL_Log("Can't creat texture, %s", SDL_GetError());
        return -2;
    }
    if (0 != SDL_RenderCopy(g_render, texture, &rect.src, &rect.dst)) {
        SDL_Log("Can't copy render, %s", SDL_GetError());
        return -3;
    }
    SDL_RenderPresent(g_render);
    SDL_DestroyTexture(texture);
#endif
    return 0;
}

void SDL_GL_showText(const char* text, TextCfg cfg)
{
    TTF_Font* font = TTF_OpenFont(cfg.font, 16);
    if (font != NULL) {
        TTF_SetFontStyle(font, cfg.style);
        SDL_Surface* message = TTF_RenderUTF8_Solid(font, text, cfg.color);
        if (message != NULL) {
            //Temporary rectangle to hold the offsets
            // SDL_BlitSurface(message, NULL, g_surface, &cfg.rect.dst);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(g_render, message);
            if (texture == NULL) {
                SDL_Log("Can't creat texture, %s", SDL_GetError());
                return;
            }
            if (0 != SDL_RenderCopy(g_render, texture, &cfg.rect.src, &cfg.rect.dst)) {
                SDL_Log("Can't copy render, %s", SDL_GetError());
                return;
            }
            SDL_RenderPresent(g_render);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(message);
        }
        TTF_CloseFont(font);
    } else {
        SDL_Log("TTF_OpenFont: Open simsun.ttf %s\n", TTF_GetError());
    }
}

void SDL_GL_SetAs2DTex(int w, int h)
{
    /* Note, there may be other things you need to change,
    depending on how you have your OpenGL state set up.
    */
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    /* This allows alpha blending of 2D textures with the scene */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (GLdouble)w, (GLdouble)h, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void SDL_GL_Leave2DTex()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

/* Quick utility function for texture creation */
static int expandPower2(int input)
{
    int value = 1;

    while (value < input) {
        value <<= 1;
    }
    return value;
}

GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* texcoord)
{
    GLuint texture = 0;
    int w, h;

    /* Use the surface width and height expanded to powers of 2 */
    w = expandPower2(surface->w);
    h = expandPower2(surface->h);
    texcoord[0] = 0.0f;         /* Min X */
    texcoord[1] = 0.0f;         /* Min Y */
    texcoord[2] = (GLfloat)surface->w / w;   /* Max X */
    texcoord[3] = (GLfloat)surface->h / h;   /* Max Y */

#if SDL_MAJOR_VERSION < 2
    SDL_Surface* image;
    SDL_Rect area;
    Uint32 saved_flags;
    Uint8  saved_alpha;

    image = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        w, h,
        32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        0xFF000000
#else
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x000000FF
#endif
    );
    if (image == NULL) {
        return 0;
    }
    /* Save the alpha blending attributes */
    saved_flags = surface->flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
#if SDL_VERSION_ATLEAST(1, 3, 0)
    SDL_GetSurfaceAlphaMod(surface, &saved_alpha);
    SDL_SetSurfaceAlphaMod(surface, 0xFF);
#else
    saved_alpha = surface->format->alpha;
    if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        SDL_SetAlpha(surface, 0, 0);
    }
#endif

    /* Copy the surface into the GL texture image */
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    SDL_BlitSurface(surface, &area, image, &area);

    /* Restore the alpha blending attributes */
#if SDL_VERSION_ATLEAST(1, 3, 0)
    SDL_SetSurfaceAlphaMod(surface, saved_alpha);
#else
    if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        SDL_SetAlpha(surface, saved_flags, saved_alpha);
    }
#endif
    /* Create an OpenGL texture for the image */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,
        w, h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image->pixels);
    SDL_FreeSurface(image); /* No longer needed */
#else
    SDL_Log("texCoords = [%.3f, %.3f, %.3f, %.3f]", texcoord[0], texcoord[1], texcoord[2], texcoord[3]);
#endif
    return texture;
}

void SDL_GL_quit()
{
#if SDL_MAJOR_VERSION >= 2
    TTF_Quit();
    SDL_FreeSurface(g_surface);
    SDL_RenderClear(g_render);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
#endif
}
