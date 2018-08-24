#include "GlF16.h"
/*
* Globals...
*/
double     LastTime;          /* Last update time */
GLuint     F16Body,           /* F-16 body */
F16Rolleron[2];    /* F-16 rollerons */
GLuint     F16Texture[3];     /* Camoflage texture objects */
int        UseTexturing = 1;  /* Use texture mapping? */
GLfloat	   Orient[3] =   /* Orientation of viewer */
{
    15.0, 45.0, 30.0
};
/*
* 'glu_vertex()' - Set a vertex.
*/

void APIENTRY
glu_vertex(GLdouble *xyz)  /* I - XYZ location + ST texture coordinate */
{
    glTexCoord2dv(xyz + 3);
    glVertex3dv(xyz);
}

/*
* 'BuildF16()' - Build the F-16 model.
*/

void
BuildF16(int wide, int tall)
{
    glViewport(0, 0, wide, tall);
    int                i;           /* Looping var */
    GLUquadric         *quadric;    /* Quadric object */
#ifdef GLU_VERSION_1_2
    GLUtesselator      *tess;       /* Tesselator object */
#else
    GLUtriangulatorObj *tess;
#endif /* GLU_VERSION_1_2 */
    static GLdouble    wing[][5] =  /* Main wing points */
    {
        /* { x, y, z, s, t } */
        { 0.25, 0.0, -1.0,  0.125, 0.0 },
        { 0.45, 0.0,  0.0,  0.25,  0.4 },
        { 1.65, 0.0,  0.8,  1.0,   0.8 },
        { 1.65, 0.0,  1.2,  1.0,   1.0 },
        { 0.35, 0.0,  1.2,  0.15,  1.0 },
        { 0.35, 0.0,  2.4,  0.15,  2.0 },
        { 0.25, 0.0,  2.4,  0.125, 2.0 },
        { 0.25, 0.0,  2.0,  0.125, 1.5 },
        { -0.25, 0.0,  2.0, -0.125, 1.5 },
        { -0.25, 0.0,  2.4, -0.125, 2.0 },
        { -0.35, 0.0,  2.4, -0.15,  2.0 },
        { -0.35, 0.0,  1.2, -0.15,  1.0 },
        { -1.65, 0.0,  1.2, -1.0,   1.0 },
        { -1.65, 0.0,  0.8, -1.0,   0.8 },
        { -0.45, 0.0,  0.0, -0.25,  0.4 },
        { -0.25, 0.0, -1.0, -0.125, 0.0 }
    };
    static GLdouble    tail[][5] =      /* Tail points */
    {
        /* { x, y, z, s, t } */
        { 0.0, 0.24, 0.5, 1.5, 0.0 },
        { 0.0, 0.4,  1.1, 1.2, 0.1 },
        { 0.0, 1.0,  2.0, 0.4, 1.0 },
        { 0.0, 1.0,  2.4, 0.05, 1.0 },
        { 0.0, 0.4,  2.1, 0.2, 0.1 },
        { 0.0, 0.24, 2.1, 0.2, 0.0 }
    };
    static GLdouble    left_fin[][5] =  /* Left fin points */
    {
        /* { x, y, z, s, t } */
        { -0.1,  -0.1, 1.1, 0.0, 0.0 },
        { -0.25, -0.3, 1.2, 1.0, 0.0 },
        { -0.25, -0.3, 1.5, 1.0, 1.0 },
        { -0.1,  -0.1, 1.5, 1.0, 0.0 }
    };
    static GLdouble    right_fin[][5] = /* Right fin points */
    {
        /* { x, y, z, s, t } */
        { 0.1,  -0.1, 1.1, 0.0, 0.0 },
        { 0.25, -0.3, 1.2, 1.0, 0.0 },
        { 0.25, -0.3, 1.5, 1.0, 1.0 },
        { 0.1,  -0.1, 1.5, 1.0, 0.0 }
    };
    static GLdouble    left_rolleron[][5] =  /* Left rolleron points */
    {
        /* { x, y, z, s, t } */
        { -0.35, 0.0, 1.6, 0.0, 0.0 },
        { -0.85, 0.0, 2.1, 1.0, 0.5 },
        { -0.85, 0.0, 2.4, 1.0, 1.0 },
        { -0.35, 0.0, 2.4, 0.0, 1.0 }
    };
    static GLdouble    right_rolleron[][5] = /* Right rolleron points */
    {
        /* { x, y, z, s, t } */
        { 0.35, 0.0, 1.6, 0.0, 0.0 },
        { 0.85, 0.0, 2.1, 1.0, 0.5 },
        { 0.85, 0.0, 2.4, 1.0, 1.0 },
        { 0.35, 0.0, 2.4, 0.0, 1.0 }
    };

    /* Load the texture images */
    F16Texture[0] = TextureLoad("image/camoflage.bmp", GL_FALSE, GL_NEAREST,
        GL_NEAREST, GL_REPEAT);
    F16Texture[1] = TextureLoad("image/TAIL.BMP", GL_FALSE, GL_NEAREST,
        GL_NEAREST, GL_REPEAT);

    /* Then build the F-16 body */
    F16Body = glGenLists(1);
    glNewList(F16Body, GL_COMPILE);
    glBindTexture(GL_TEXTURE_2D, F16Texture[0]);

    tess = gluNewTess();
    gluTessCallback(tess, GLU_TESS_BEGIN, (void(__stdcall *)())glBegin);
    gluTessCallback(tess, GLU_TESS_END, glEnd);
    gluTessCallback(tess, GLU_TESS_VERTEX, (void (CALLBACK *)())glu_vertex);

    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);

    /* Main fuselage */
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0, 0.0, -1.5);
    gluCylinder(quadric, 0.25, 0.25, 3.5, 20, 2);
    glPopMatrix();

    /* Nose */
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0, 0.0, -2.5);
    gluCylinder(quadric, 0.0, 0.25, 1.0, 20, 2);
    glPopMatrix();

    /* Main wing */
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(0.0, 1.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 16; i++)
        gluTessVertex(tess, wing[i], wing[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    /* Fins */
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(-1.0, 0.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 4; i++)
        gluTessVertex(tess, left_fin[i], left_fin[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(1.0, 0.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 4; i++)
        gluTessVertex(tess, right_fin[i], right_fin[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    /* Tail */
    glBindTexture(GL_TEXTURE_2D, F16Texture[1]);
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(1.0, 0.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 6; i++)
        gluTessVertex(tess, tail[i], tail[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    /* Don't texture any more of the body... */
    glDisable(GL_TEXTURE_2D);

    /* Canopy */
    glPushMatrix();
    glColor4f(0.5, 0.5, 1.0, 0.75);
    glTranslatef(0.0f, 0.2f, -1.0f);
    glScalef(1.0f, 1.0f, 0.65f / 0.15f);
    gluSphere(quadric, 0.15, 6, 12);
    glPopMatrix();

    /* Engine */
    glPushMatrix();
    /* Cowling */
    glColor3f(0.1f, 0.1f, 0.1f);
    glTranslatef(0.0, 0.0, 2.0);
    gluCylinder(quadric, 0.25, 0.15, 0.5, 20, 2);
    gluDisk(quadric, 0.0, 0.25, 20, 2);

    /* Exhaust */
    glPushAttrib(GL_LIGHTING_BIT);
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    glColor3f(0.5, 0.5, 1.0);
    gluCylinder(quadric, 0.2, 0.0, 0.3, 10, 2);
    glColor4f(0.25, 0.25, 1.0, 0.75);
    gluCylinder(quadric, 0.2, 0.1, 0.4, 10, 2);
    glPopAttrib();
    glPopMatrix();

    glEndList();

    /* Now the left rolleron */
    F16Rolleron[0] = glGenLists(1);
    glNewList(F16Rolleron[0], GL_COMPILE);

    glBindTexture(GL_TEXTURE_2D, F16Texture[0]);
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(0.0, 1.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 4; i++)
        gluTessVertex(tess, left_rolleron[i], left_rolleron[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    glEndList();

    /* And the right rolleron */
    F16Rolleron[1] = glGenLists(1);
    glNewList(F16Rolleron[1], GL_COMPILE);

    glBindTexture(GL_TEXTURE_2D, F16Texture[0]);
    glColor3f(0.8f, 0.8f, 0.8f);
    glNormal3f(0.0, 1.0, 0.0);
    gluTessBeginPolygon(tess, NULL);
#ifdef GL_VERSION_1_2
    gluTessBeginContour(tess);
#endif /* GL_VERSION_1_2 */

    for (i = 0; i < 4; i++)
        gluTessVertex(tess, right_rolleron[i], right_rolleron[i]);

#ifdef GL_VERSION_1_2
    gluTessEndContour(tess);
#endif /* GL_VERSION_1_2 */
    gluTessEndPolygon(tess);

    glEndList();

    gluDeleteQuadric(quadric);
    gluDeleteTess(tess);
}


/*
* 'AdjustModel()' - Adjust the Model...
*/

void
AdjustModel()
{
    GLfloat        pitch, roll; /* Pitch and roll control values */
    static GLfloat sunpos[4] = { 0.7071f, 0.7071f, 0.0, 0.0 };
    static GLfloat suncolor[4] = { 0.5f, 0.5f, 0.4f, 1.0f };
    static GLfloat sunambient[4] = { 0.5f, 0.5f, 0.4f, 1.0f };

    /* Clear the window to light blue... */
    //glClearColor(0.75f, 0.75f, 1.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Setup viewing transformations for the current orientation... */
    glPushMatrix();
    glTranslatef(0.0, 0.0, -15.0);
    glRotatef(Orient[1], 0.0, -1.0, 0.0);
    glRotatef(Orient[0], 1.0, 0.0, 0.0);
    glRotatef(Orient[2], 0.0, 0.0, -1.0);

    /* Setup lighting if needed... */
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sunambient);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, sunpos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, suncolor);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunambient);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (UseTexturing)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);

    // Draw the main body
    glCallList(F16Body);

    // Draw the rollerons...
    roll = 0.1f * (_dX);
    pitch = 0.1f * (_dY);

    if (UseTexturing)
        glEnable(GL_TEXTURE_2D);

    // Left rolleron
    glPushMatrix();
    glTranslatef(0.0, 0.0, 2.0);
    glRotatef(roll - pitch, 1.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, -2.0);
    glCallList(F16Rolleron[0]);
    glPopMatrix();

    // Right rolleron
    glPushMatrix();
    glTranslatef(0.0, 0.0, 2.0);
    glRotatef(roll + pitch, -1.0, 0.0, 0.0);
    glTranslatef(0.0, 0.0, -2.0);
    glCallList(F16Rolleron[1]);
    glPopMatrix();

    glPopMatrix();

    /* Finish up */
    //glutSwapBuffers();
}
