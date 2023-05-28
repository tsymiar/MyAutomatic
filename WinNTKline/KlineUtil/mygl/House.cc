#include "House.h"

extern	TEXTURE_2D** TextureList;

House::House() {}

// Create an OpenGL rendering context
BOOL House::CreateViewGLContext(HDC hDC)
{
    m_hGLContext = wglCreateContext(hDC);

    if (m_hGLContext == NULL)
        return FALSE;

    if (wglMakeCurrent(hDC, m_hGLContext) == FALSE)
        return FALSE;

    return TRUE;
}

void House::InitGeometry(void)
{
    GLfloat fogColor[4] = { 0.75, 0.75, 1.0, 1.0 };

    speed = 0;
    srand(224);
    srand((unsigned)time(NULL));
    // Default mode
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    glShadeModel(GL_FLAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.8f);
    glFogf(GL_FOG_START, 400.0f);
    glFogf(GL_FOG_END, 500.0f);
    glClearColor(0.75f, 0.75f, 1.0f, 1.0f);
    // light must be disabled 
    // while rendering the terrain
    // because it has no normal definition
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
}

// Function that moves the eye or turns the angle of sight.
// Updates scene if update != 0.
void House::MoveEye(int type, GLfloat amount, int update)
{
    GLfloat a;
    switch (type) {
    case FORWARD:
        a = sqrt((cx - ex) * (cx - ex) + (cz - ez) * (cz - ez));
        ex = (amount * (cx - ex) + a * ex) / a;
        ez = (amount * (cz - ez) + a * ez) / a;
        cx = (amount * (cx - ex) + a * cx) / a;
        cz = (amount * (cz - ez) + a * cz) / a;
        break;
    case MOVELR:
        a = sqrt((cx - ex) * (cx - ex) + (cy - ey) * (cy - ey));
        ex = (amount * (cx - ex) + a * ex) / a;
        ey = (amount * (cy - ey) + a * ey) / a;
        cx = (amount * (cx - ex) + a * cx) / a;
        cy = (amount * (cy - ey) + a * cy) / a;
    case TURNLEFT:
        cx = (cx - ex) * (float)cos(amount / 360.0f) + (cz - ez) * (float)sin(amount / 360.0f) + ex;
        cz = (cz - ez) * (float)cos(amount / 360.0f) - (cx - ex) * (float)sin(amount / 360.0f) + ez;
        break;
    case UPDOWN:
        ey += amount;
        break;
    case LOOKUP:
        cy += amount;
        break;
    case DEFAULT:
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
        break;
    }
    if (update) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(ex, ey, ez, cx, cy, cz, ux, uy, uz);
    }
}

void  House::ReadData()
{
    int  i;
    unsigned  j, l;
    FILE* fp;
    char    stemp[100];
    POINT3D* plist;
    INT4U   nAllVertexNum;
    INT4U* pchlist;

    strcpy_s(gEnergyFile, "data/ROOM.ED");
    fopen_s(&fp, gEnergyFile, "r");
    if (fp == NULL) {
        printf("\n Can not open energy data file:%s\n", gEnergyFile);
        exit(0);
    }
    fseek(fp, 0, SEEK_SET);

    /******  read texture list   ******/
    fscanf_s(fp, "%s", stemp, sizeof(stemp));

    while (strcmp(stemp, "texnum") != 0)  fscanf_s(fp, "%s", stemp, sizeof(stemp));
    fscanf_s(fp, "%d", &texNum);

    TextureList = (TEXTURE_2D**)malloc(sizeof(TEXTURE_2D) * (texNum + 1));
    for (i = 1; i <= texNum; i++) {
        TextureList[i] = (TEXTURE_2D*)malloc(sizeof(TEXTURE_2D));
        fscanf_s(fp, "%s%s", TextureList[i]->fname, sizeof(TextureList[i]->fname), stemp, sizeof(stemp));
        if (strcmp(stemp, "REPEAT_TEXTURE") == 0)
            TextureList[i]->type = 1;
        else  if (strcmp(stemp, "CLAMP_TEXTURE") == 0)
            TextureList[i]->type = 0;
    }

    /******   Read object list   ******/
    fscanf_s(fp, "%s", stemp, sizeof(stemp));

    while (strcmp(stemp, "ObjectNum") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
    fscanf_s(fp, "%ld", &ObjectNum);

    ObjectList = (OBJECT*)malloc(sizeof(OBJECT) * ObjectNum);
    for (i = 0; i < ObjectNum; i++) {
        fscanf_s(fp, "%s", stemp, sizeof(stemp));
        while (strcmp(stemp, "SurfaceNum") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
        fscanf_s(fp, "%ld", &(ObjectList[i].SurfNum));

        ObjectList[i].surflist = (SURFACE*)malloc(sizeof(SURFACE) * ObjectList[i].SurfNum);
        for (j = 0; j < ObjectList[i].SurfNum; j++) {
            /******   Read surface infor   ******/
            fscanf_s(fp, "%s", stemp, sizeof(stemp));
            while (strcmp(stemp, "TextureId") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(ObjectList[i].surflist[j].texId));

            fscanf_s(fp, "%s", stemp, sizeof(stemp));
            while (strcmp(stemp, "pointnum") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(ObjectList[i].surflist[j].pointn));

            fscanf_s(fp, "%s", stemp, sizeof(stemp));
            while (strcmp(stemp, "triangle") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(ObjectList[i].surflist[j].triangle));

            fscanf_s(fp, "%s", stemp, sizeof(stemp));
            while (strcmp(stemp, "quadrangle") != 0) fscanf_s(fp, "%s", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(ObjectList[i].surflist[j].quadric));


            /******   Read point list    ******/
            ObjectList[i].surflist[j].pointlist = (POINT3D*)malloc(sizeof(POINT3D) *
                ObjectList[i].surflist[j].pointn);

            plist = ObjectList[i].surflist[j].pointlist;

            for (l = 0; l < ObjectList[i].surflist[j].pointn; l++)
                fscanf_s(fp, "%f%f%f%f%f%f%f%f",
                    &(plist[l].r), &(plist[l].g), &(plist[l].b),
                    &(plist[l].u), &(plist[l].v),
                    &(plist[l].x), &(plist[l].y), &(plist[l].z));

            /******    Read patchlist    ******/
            nAllVertexNum = ObjectList[i].surflist[j].triangle * 3 +
                ObjectList[i].surflist[j].quadric * 4;

            ObjectList[i].surflist[j].patchlist = (INT4U*)malloc(sizeof(INT4U) * nAllVertexNum);

            pchlist = ObjectList[i].surflist[j].patchlist;

            for (l = 0; l < nAllVertexNum; l++)
                fscanf_s(fp, "%ld", &(pchlist[l]));
        }
    }
    fclose(fp);
}

void House::InitLookAt()
{
    FILE* fp;

    strcpy_s(sLookAtFN, "data/ROOM.LK");
    fopen_s(&fp, sLookAtFN, "rb");
    if (fp == NULL) {
        ex = ey = ez = 1.0f;
        cx = cy = cz = 0.0f;
        Near = 0.1f;
        angle = 30.0f;
    } else
        fscanf_s(fp, "%f%f%f%f%f%f%f%f", &angle, &Near, &ex, &ey, &ez, &cx, &cy, &cz);
    fclose(fp);
}

void	House::InitRenderWin()
{
    glShadeModel(GL_SMOOTH);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluPerspective(angle, (float)Width / (float)Height, Near, 1000000000.0);
    gluLookAt(ex, ey, ez, cx, cy, cz, 0.0, 1.0, 0.0);
}

void	House::Render(void)
{
    int     	i;
    unsigned  j, k, l, m, TexIndex;
    POINT3D* plist;
    INT4U* pchlist;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
    for (i = 0; i < ObjectNum; i++)
        for (j = 0; j < ObjectList[i].SurfNum; j++) {
            TexIndex = ObjectList[i].surflist[j].texId;
            if (TexIndex > 0)
                InitTex(TexIndex);
            plist = ObjectList[i].surflist[j].pointlist;
            pchlist = ObjectList[i].surflist[j].patchlist;

            l = 0;
            for (k = 0; k < ObjectList[i].surflist[j].triangle; k++) {
                glBegin(GL_TRIANGLES);
                for (m = 0; m < 3; m++) {
                    glColor3f(plist[pchlist[l]].r,
                        plist[pchlist[l]].g,
                        plist[pchlist[l]].b);
                    glTexCoord2f(plist[pchlist[l]].u,
                        plist[pchlist[l]].v);
                    glVertex3f(plist[pchlist[l]].x,
                        plist[pchlist[l]].y,
                        plist[pchlist[l]].z);
                    l++;
                }/* m */
                glEnd();

            }/* k */

            for (k = 0; k < ObjectList[i].surflist[j].quadric; k++) {
                glBegin(GL_QUADS);
                for (m = 0; m < 4; m++) {
                    glColor3f(plist[pchlist[l]].r,
                        plist[pchlist[l]].g,
                        plist[pchlist[l]].b);
                    glTexCoord2f(plist[pchlist[l]].u,
                        plist[pchlist[l]].v);
                    glVertex3f(plist[pchlist[l]].x,
                        plist[pchlist[l]].y,
                        plist[pchlist[l]].z);
                    l++;
                }/* m */
                glEnd();
            }/* k */
            glFlush();
            KillTex();
        }
}

void House::CleanList()
{
    int i;
    unsigned  j;

    for (i = 0; i < ObjectNum; i++) {
        for (j = 0; j < ObjectList[i].SurfNum; j++) {
            free(ObjectList[i].surflist[j].pointlist);
            free(ObjectList[i].surflist[j].patchlist);
        }
        free(ObjectList[i].surflist);
    }
    free(ObjectList);
    for (i = 1; i <= texNum; i++)
        free(TextureList[i]);
    free(TextureList);
}

/********************************/
/*	function : OpenTexImage	*/
/********************************/
unsigned char* House::OpenTexImage(INT2U TexIndex, INT2U* rslx, INT2U* rsly)
{
    unsigned char* image;
    FILE* fp;
    INT2U		srcx, srcy;
    INT4U		i, j;
    char		ImageName[30];
    unsigned char* SImageData;
    int width, height;

    strcpy_s(ImageName, TextureList[TexIndex]->fname);

    /* load a image */
    fopen_s(&fp, ImageName, "rb");
    if (!fp) return 0;
    fseek(fp, 18L, 0);
    fread(&width, sizeof(long), 1, fp);
    fread(&height, sizeof(long), 1, fp);
    *rslx = srcx = width; *rsly = srcy = height;
    fseek(fp, 54L, 0);
    image = (unsigned char*)malloc(width * height * 3);
    fread(image, width * height * 3, 1, fp);
    fclose(fp);
    SImageData = (unsigned char*)malloc(srcx * srcy * 3);
    for (i = 0; i < srcx; i++) {
        for (j = 0; j < srcy; j++) {
            (unsigned char)*(SImageData + i * srcx * 3 + j * 3 + 0) = (unsigned char)*(image + i * srcx * 3 + j * 3 + 2);
            (unsigned char)*(SImageData + i * srcx * 3 + j * 3 + 1) = (unsigned char)*(image + i * srcx * 3 + j * 3 + 1);
            (unsigned char)*(SImageData + i * srcx * 3 + j * 3 + 2) = (unsigned char)*(image + i * srcx * 3 + j * 3 + 0);
        }
    }
    free(image);
    printf("%s : %d=%ul\n", ImageName, srcx * srcy * 3, (unsigned int)(i * j * 3));
    return(SImageData);
}

/********************************/
/*	function : InitTex	*/
/********************************/
void	House::InitTex(int TexIndex)
{
    INT2U  TextType;
    unsigned char* ImageData;
    static	int	OldIndex = -1;

    if (TexIndex <= 0) return;
    if (TexIndex == OldIndex) {
        glEnable(GL_TEXTURE_2D);
        return;
    }

    ImageData = ImageDatas[TexIndex - 1];

    TextType = TextureList[TexIndex]->type;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    if (TextType == CLAMP_TEXTURE) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    } else {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, 3, rslxs[TexIndex - 1], rslys[TexIndex - 1],
        0, GL_RGB, GL_UNSIGNED_BYTE, ImageData);
    glEnable(GL_TEXTURE_2D);
    OldIndex = TexIndex;
}

/********************************/
/*	function : KillTex	*/
/********************************/
void	House::KillTex()
{
    glDisable(GL_TEXTURE_2D);
}

void House::LoadAllTexture()
{
    int i;

    for (i = 0; i < texNum; i++)
        ImageDatas[i] = OpenTexImage(i + 1, &rslxs[i], &rslys[i]);
}

void House::CleanAllTexture()
{
    for (int i = 0; i < texNum; i++)
        free(ImageDatas[i]);
}

House::~House()
{
}

#ifdef __error //ws2tcpip.h 'Error' redefined.
#undef __error
#endif
