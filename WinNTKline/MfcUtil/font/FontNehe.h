#ifndef FONT_NEHE_H_
#define FONT_NEHE_H_

// FreeType Headers
#include <font/ft2build.h>
#include <font/freetype/freetype.h>
#include <font/freetype/ftglyph.h>
#include <font/freetype/ftoutln.h>
#include <font/freetype/fttrigon.h>

// OpenGL Headers
#include <windows.h>                                      // (The GL Headers Need It)
#include <GL/gl.h>
#include <GL/glu.h>

// Some STL Headers
#include <vector>
#include <string>

// Using The STL Exception Library Increases The
// Chances That Someone Else Using Our Code Will Correctly
// Catch Any Exceptions That We Throw.
#include <stdexcept>
#pragma comment(lib, "freetype.lib") 
// MSVC Will Spit Out All Sorts Of Useless Warnings If
// You Create Vectors Of Strings, This Pragma Gets Rid Of Them.
#pragma warning(disable: 4244)
#pragma warning(disable: 4786)
#pragma warning(disable: 4793)
// Wrap Everything In A Namespace, That Way We Can Use A Common
// Function Name Like "print" Without Worrying About
// Overlapping With Anyone Else's Code.
namespace freetype {

    // Inside Of This Namespace, Give Ourselves The Ability
    // To Write Just "vector" Instead Of "std::vector"
    using std::vector;

    // Ditto For String.
    using std::string;

    // This Holds All Of The Information Related To Any
    // FreeType Font That We Want To Create. 
    struct font_data {
        float h;                                        // Holds The Height Of The Font.
        GLuint * textures;                                  // Holds The Texture Id's
        GLuint list_base;                                   // Holds The First Display List Id

                                                            // The Init Function Will Create A Font With
                                                            // The Height h From The File fname.
        void init(const char * fname, unsigned int h);

        // Free All The Resources Associated With The Font.
        void clean();
    };
    // The Flagship Function Of The Library - This Thing Will Print
    // Out Text At Window Coordinates X, Y, Using The Font ft_font.
    // The Current Modelview Matrix Will Also Be Applied To The Text.
    __declspec(dllexport) void textout(const font_data &ft_font, float x, float y, const char *fmt, ...);
    int DrawGLScene(GLvoid);
}                                               // Close The Namespace
typedef struct FT_LibraryRec_
{
    FT_Memory memory; /* library's memory manager */

    FT_Int version_major;
    FT_Int version_minor;
    FT_Int version_patch;

    FT_UInt num_modules;
    FT_Module modules[FT_MAX_MODULES]; /* module objects */

    FT_ListRec renderers; /* list of renderers */
    FT_Renderer cur_renderer; /* current outline renderer */
    FT_Module auto_hinter;

    FT_Byte* raster_pool; /* scan-line conversion */
                          /* render pool */
    FT_ULong raster_pool_size; /* size of render pool in bytes */

    //FT_DebugHook_Func debug_hooks[4];

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
    //FT_LcdFilter lcd_filter;
    FT_Int lcd_extra; /* number of extra pixels */
    FT_Byte lcd_weights[7]; /* filter weights, if any */
    //FT_Bitmap_LcdFilterFunc lcd_filter_func; /* filtering callback */
#endif

} FT_LibraryRec;

#endif // !FONT_NEHE_H_
