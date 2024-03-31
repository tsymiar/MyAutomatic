#include "SDL_text.h"

static SDL_Surface *g_newSurface = NULL;

int SDL_GL_init()
{
#if SDL_MAJOR_VERSION < 2
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        return -1;
#else
    // static SDL_Surface * imgSurface = SDLTest_ImageBlit(); // sdl3
    // g_newSurface = SDL_CreateSurface(imgSurface->w, imgSurface->h, SDL_PIXELFORMAT_RGBA32);
    if (g_newSurface == NULL) {
        return -1;
    }
#endif
	//Initialize SDL_ttf
	return TTF_Init();
}

void SDL_GL_SetAs2DMode(int w, int h)
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

void SDL_GL_Leave2DMode()
{
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}

/* Quick utility function for texture creation */
static int power_of_two(int input)
{
	int value = 1;

	while (value < input) {
		value <<= 1;
	}
	return value;
}

GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord)
{
    GLuint texture;
#if SDL_MAJOR_VERSION < 2
	int w, h;
	SDL_Surface *image;
	SDL_Rect area;
	Uint32 saved_flags;
	Uint8  saved_alpha;

	/* Use the surface width and height expanded to powers of 2 */
	w = power_of_two(surface->w);
	h = power_of_two(surface->h);
	texcoord[0] = 0.0f;         /* Min X */
	texcoord[1] = 0.0f;         /* Min Y */
	texcoord[2] = (GLfloat)surface->w / w;   /* Max X */
	texcoord[3] = (GLfloat)surface->h / h;   /* Max Y */

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
    saved_flags = surface->flags&(SDL_SRCALPHA | SDL_RLEACCELOK);
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
    // new interface instead.
#endif
	return texture;
}
