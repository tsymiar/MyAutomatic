#include "House.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

extern TEXTURE_2D** TextureList;

House::House() : ex(1.0f), ey(1.0f), ez(1.0f), cx(0.0f), cy(0.0f), cz(0.0f), Near(0.1f), angle(30.0f), m_lxs(0) { }

BOOL House::CreateViewGLContext(HDC hDC)
{
    m_hGLContext = wglCreateContext(hDC);
    if (m_hGLContext == NULL || wglMakeCurrent(hDC, m_hGLContext) == FALSE)
        return FALSE;
    return TRUE;
}

void House::InitGeometry(void)
{
    GLfloat fogColor[4] = { 0.75, 0.75, 1.0, 1.0 };

    speed = 0;
    srand((unsigned)time(NULL));
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
        MoveForward(amount);
        break;
    case MOVELR:
        MoveLeftRight(amount);
        break;
    case TURNLEFT:
        TurnLeft(amount);
        break;
    case UPDOWN:
        ey += amount;
        break;
    case LOOKUP:
        cy += amount;
        break;
    case DEFAULT:
        ResetView();
        break;
    }
    if (update) {
        UpdateView();
    }
}

void House::MoveForward(GLfloat amount)
{
    GLfloat a = sqrt((cx - ex) * (cx - ex) + (cz - ez) * (cz - ez));
    ex = (amount * (cx - ex) + a * ex) / a;
    ez = (amount * (cz - ez) + a * ez) / a;
    cx = (amount * (cx - ex) + a * cx) / a;
    cz = (amount * (cz - ez) + a * cz) / a;
}

void House::MoveLeftRight(GLfloat amount)
{
    GLfloat a = sqrt((cx - ex) * (cx - ex) + (cy - ey) * (cy - ey));
    ex = (amount * (cx - ex) + a * ex) / a;
    ey = (amount * (cy - ey) + a * ey) / a;
    cx = (amount * (cx - ex) + a * cx) / a;
    cy = (amount * (cy - ey) + a * cy) / a;
}

void House::TurnLeft(GLfloat amount)
{
    cx = (cx - ex) * cos(amount / 360.0f) + (cz - ez) * sin(amount / 360.0f) + ex;
    cz = (cz - ez) * cos(amount / 360.0f) - (cx - ex) * sin(amount / 360.0f) + ez;
}

void House::ResetView()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

void House::UpdateView()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(ex, ey, ez, cx, cy, cz, ux, uy, uz);
}

void House::ReadUntil(FILE* fp, const char* dst, char* buff, size_t size)
{
    fscanf_s(fp, "%s", buff, size);
    while (strcmp(buff, dst) != 0) {
        fscanf_s(fp, "%s", buff, size);
    }
}

void House::ReadData()
{
    FILE* fp;
    char stemp[100];

    strcpy_s(gEnergyFile, "data/ROOM.ED");
    fopen_s(&fp, gEnergyFile, "r");
    if (fp == NULL) {
        printf("\n Can not open energy data file:%s\n", gEnergyFile);
        exit(0);
    }
    fseek(fp, 0, SEEK_SET);

    /******  read texture list   ******/
    ReadUntil(fp, "texnum", stemp, sizeof(stemp));
    fscanf_s(fp, "%d", &texNum);

    TextureList = (TEXTURE_2D**)malloc(sizeof(TEXTURE_2D) * (texNum + 1));
    for (int i = 1; i <= texNum; i++) {
        TextureList[i] = (TEXTURE_2D*)malloc(sizeof(TEXTURE_2D));
        fscanf_s(fp, "%s%s", TextureList[i]->fname, sizeof(TextureList[i]->fname), stemp, sizeof(stemp));
        TextureList[i]->type = (strcmp(stemp, "REPEAT_TEXTURE") == 0) ? 1 : 0;
    }

    /******   Read object list   ******/
    ReadUntil(fp, "ObjectNum", stemp, sizeof(stemp));
    fscanf_s(fp, "%ld", &objectNum);

    objectList = (OBJECT*)malloc(sizeof(OBJECT) * objectNum);
    for (int i = 0; i < objectNum; i++) {
        ReadUntil(fp, "SurfaceNum", stemp, sizeof(stemp));
        fscanf_s(fp, "%ld", &(objectList[i].SurfNum));

        objectList[i].surfaceList = (SURFACE*)malloc(sizeof(SURFACE) * objectList[i].SurfNum);
        for (unsigned j = 0; j < objectList[i].SurfNum; j++) {
            /******   Read surface infor   ******/
            ReadUntil(fp, "TextureId", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(objectList[i].surfaceList[j].texId));

            ReadUntil(fp, "pointnum", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(objectList[i].surfaceList[j].points));

            ReadUntil(fp, "triangle", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(objectList[i].surfaceList[j].triangle));

            ReadUntil(fp, "quadrangle", stemp, sizeof(stemp));
            fscanf_s(fp, "%d", &(objectList[i].surfaceList[j].quadric));
            /******   Read point list    ******/
            objectList[i].surfaceList[j].pointlist = (POINT3D*)malloc(sizeof(POINT3D) * objectList[i].surfaceList[j].points);
            POINT3D* plist = objectList[i].surfaceList[j].pointlist;

            for (unsigned l = 0; l < objectList[i].surfaceList[j].points; l++)
                fscanf_s(fp, "%f%f%f%f%f%f%f%f",
                    &(plist[l].r), &(plist[l].g), &(plist[l].b),
                    &(plist[l].u), &(plist[l].v),
                    &(plist[l].x), &(plist[l].y), &(plist[l].z));

            /******    Read patchlist    ******/
            INT4U nAllVertexNum = objectList[i].surfaceList[j].triangle * 3 + objectList[i].surfaceList[j].quadric * 4;
            objectList[i].surfaceList[j].patchlist = (INT4U*)malloc(sizeof(INT4U) * nAllVertexNum);
            INT4U* pch = objectList[i].surfaceList[j].patchlist;

            for (unsigned l = 0; l < nAllVertexNum; l++)
                fscanf_s(fp, "%ld", &(pch[l]));
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
    } else {
        fscanf_s(fp, "%f%f%f%f%f%f%f%f", &angle, &Near, &ex, &ey, &ez, &cx, &cy, &cz);
        fclose(fp);
    }
}

void House::InitRenderWin()
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

void House::Render(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
    for (int i = 0; i < objectNum; i++) {
        for (unsigned j = 0; j < objectList[i].SurfNum; j++) {
            unsigned TexIndex = objectList[i].surfaceList[j].texId;
            if (TexIndex > 0)
                InitTex(TexIndex);
            POINT3D* plist = objectList[i].surfaceList[j].pointlist;
            INT4U* pch = objectList[i].surfaceList[j].patchlist;

            unsigned l = 0;
            for (unsigned k = 0; k < objectList[i].surfaceList[j].triangle; k++) {
                glBegin(GL_TRIANGLES);
                for (unsigned m = 0; m < 3; m++) {
                    glColor3f(plist[pch[l]].r, plist[pch[l]].g, plist[pch[l]].b);
                    glTexCoord2f(plist[pch[l]].u, plist[pch[l]].v);
                    glVertex3f(plist[pch[l]].x, plist[pch[l]].y, plist[pch[l]].z);
                    l++;
                }
                glEnd();
            }

            for (unsigned k = 0; k < objectList[i].surfaceList[j].quadric; k++) {
                glBegin(GL_QUADS);
                for (unsigned m = 0; m < 4; m++) {
                    glColor3f(plist[pch[l]].r, plist[pch[l]].g, plist[pch[l]].b);
                    glTexCoord2f(plist[pch[l]].u, plist[pch[l]].v);
                    glVertex3f(plist[pch[l]].x, plist[pch[l]].y, plist[pch[l]].z);
                    l++;
                }
                glEnd();
            }
            glFlush();
            KillTex();
        }
    }
}

void House::CleanList()
{
    for (int i = 0; i < objectNum; i++) {
        for (unsigned j = 0; j < objectList[i].SurfNum; j++) {
            free(objectList[i].surfaceList[j].pointlist);
            free(objectList[i].surfaceList[j].patchlist);
        }
        free(objectList[i].surfaceList);
    }
    free(objectList);
    for (int i = 1; i <= texNum; i++)
        free(TextureList[i]);
    free(TextureList);
}

/********************************/
/*	function : OpenTexImage	*/
/********************************/
unsigned char* House::OpenTexImage(INT2U TexIndex, INT2U* slx, INT2U* sly)
{
    unsigned char* image;
    FILE* fp;
    INT2U srcx, srcy;
    char ImageName[30];
    unsigned char* SImageData;
    int width, height;

    strcpy_s(ImageName, TextureList[TexIndex]->fname);
    fopen_s(&fp, ImageName, "rb");
    if (!fp) return 0;
    fseek(fp, 18L, 0);
    fread(&width, sizeof(long), 1, fp);
    fread(&height, sizeof(long), 1, fp);
    *slx = srcx = width; *sly = srcy = height;
    fseek(fp, 54L, 0);
    image = (unsigned char*)malloc(width * height * 3);
    fread(image, width * height * 3, 1, fp);
    fclose(fp);
    SImageData = (unsigned char*)malloc(srcx * srcy * 3);
    for (INT4U i = 0; i < srcx; i++) {
        for (INT4U j = 0; j < srcy; j++) {
            SImageData[i * srcx * 3 + j * 3 + 0] = image[i * srcx * 3 + j * 3 + 2];
            SImageData[i * srcx * 3 + j * 3 + 1] = image[i * srcx * 3 + j * 3 + 1];
            SImageData[i * srcx * 3 + j * 3 + 2] = image[i * srcx * 3 + j * 3 + 0];
        }
    }
    free(image);
    printf("%s : %d=%ul\n", ImageName, srcx * srcy * 3, (unsigned int)(srcx * srcy * 3));
    return SImageData;
}

/********************************/
/*	function : InitTex	*/
/********************************/
void House::InitTex(int TexIndex)
{
    if (TexIndex <= 0) return;
    static int OldIndex = -1;
    if (TexIndex == OldIndex) {
        glEnable(GL_TEXTURE_2D);
        return;
    }

    unsigned char* ImageData = ImageDatas[TexIndex - 1];
    INT2U TextType = TextureList[TexIndex]->type;

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
    glTexImage2D(GL_TEXTURE_2D, 0, 3, m_lxs[TexIndex - 1], m_lys[TexIndex - 1], 0, GL_RGB, GL_UNSIGNED_BYTE, ImageData);
    glEnable(GL_TEXTURE_2D);
    OldIndex = TexIndex;
}

/********************************/
/*	function : KillTex	*/
/********************************/
void House::KillTex()
{
    glDisable(GL_TEXTURE_2D);
}

void House::LoadAllTexture()
{
    for (int i = 0; i < texNum; i++)
        ImageDatas[i] = OpenTexImage(i + 1, &m_[i], &m_lys[i]);
}

void House::CleanAllTexture()
{
    for (int i = 0; i < texNum; i++)
        free(ImageDatas[i]);
}

House::~House()
{
    CleanList();
    CleanAllTexture();
}

#ifdef __error //ws2tcpip.h 'Error' redefined.
#undef __error
#endif
