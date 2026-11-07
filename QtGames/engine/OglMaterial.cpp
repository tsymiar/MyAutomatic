#include "OglMaterial.h"
#include "SDL2tex.h"
#include <QPainter>
#include <QPen>

#ifdef _WIN32
#define IMAGE_PATH "external\\kline\\image\\"
#else
#define IMAGE_PATH "external/kline/image/"
#endif

QOglMaterial::QOglMaterial(QWidget* parent)
    : QOpenGLWidget(parent)
{
#ifdef _GLVBO_
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
#else
    xVal = yVal = zZoom = 0;
    tHigh = -1;
    mX = mY = 0;
#endif
}

QOglMaterial::~QOglMaterial()
{
    if (m_showSdl) {
        SDL_GL_quit();
    }
}

#ifdef _GLVBO_
/* 顶点着色器 */
static const char* vertexShaderSourceCore =
"attribute vec4 vPosition;\n"
"uniform highp mat4 matrix;\n"
"attribute lowp vec4 a_color;\n"
"varying lowp vec4 v_color;\n"
"void main() {\n"
"   v_color = a_color;\n"
"   gl_Position = matrix * vPosition;\n"
"}\n";

/* 片段着色器 */
static const char* fragmentShaderSourceCore =
"varying lowp vec4 v_color;\n"
"void main() {\n"
"   gl_FragColor = v_color;\n"
"}\n";

/* 2.1 图形顶点坐标 */
GLfloat vVertices[] = {
    0.0f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f
};
/* 2.2 图形顶点索引 */
GLuint tri_index[] = { 0, 1, 2 };
/* 2.3 顶点颜色数组 */
GLfloat colors[] = {
    1.0f, 0.0f, 0.0f,0.5f,
    0.0f, 1.0f, 0.0f,0.5f,
    0.0f, 0.0f, 1.0f,0.5f
};

void QOglMaterial::initVbo()
{
    glGenBuffers(1, &verVbo);
    glBindBuffer(GL_ARRAY_BUFFER, verVbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), vVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &v_indexVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v_indexVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLuint), tri_index, GL_STATIC_DRAW);

    glGenBuffers(1, &colorVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
}
#endif

void QOglMaterial::coord()
{
    glDisable(GL_DEPTH_TEST);
    QPainter painter;
    painter.begin(this);
    QPen pen;
    pen.setColor(Qt::yellow);
    painter.setPen(pen);
    QString coord = QString("(x=%1,y=%2,h=%3,gl[%4,%5])")
        .arg(mX).arg(mY).arg(tHigh)
        .arg(xVal).arg(yVal);
    painter.drawText(10, 20, coord);
    painter.end();
    glEnable(GL_DEPTH_TEST);
}

void QOglMaterial::textOut(int left, int upon, QColor color, float th, QString family)
{
    glDisable(GL_DEPTH_TEST);
    QPen pen;
    pen.setColor(color);
    QFont font;
    if (family.isNull())
        family = font.defaultFamily();
    font.setFamily(family);
    QPainter painter;
    painter.begin(this);
    painter.setPen(pen);
    painter.setFont(font);
    painter.scale(th, th);
    painter.drawText(left, upon, text);
    painter.end();
    glEnable(GL_DEPTH_TEST);
}

void QOglMaterial::initializeGL()
{
    qDebug("+++ initializeGL +++");
    initializeOpenGLFunctions();
#ifdef _GLVBO_
    program = new QOpenGLShaderProgram(this);
    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceCore)) {
        return;
    }
    if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceCore)) {
        return;
    }

    if (!program->link())
        return;
    if (!program->bind())
        return;

    programid = program->programId();
    matrixLocation = glGetUniformLocation(programid, "matrix");
    vertexLocation = glGetAttribLocation(programid, "vPosition");
    clorLocation = program->attributeLocation("a_color");

    // 对于实时变化的数据，vbo 要在 paintGL() 里每次更新
    initVbo();
#else
#ifdef  OGL_KVIEW_H_
    kv.AdjustDraw(640, 480);
#else
    mPng.setPixels(IMAGE_PATH"atlas.png");
    mPng.loadAtlasBackground(IMAGE_PATH"atlas.png");
#if SDL_MAJOR_VERSION >= 2
    if (m_showSdl && (SDL_GL_init() == 0)) {
#ifndef MAX_PATH
#define MAX_PATH 256
#endif
        char fileName[MAX_PATH];
        sprintf(fileName, "%sspabandari.bmp", IMAGE_PATH);
        struct stCopyRect rect { { 100, 10, 360, 70 }, { 70,360,500,70 } };
        SDL_GL_loadImage(fileName, rect);
        rect.src = { 128,160,360,256 };
        rect.dst = { 1,1,256,180 };
        SDL_GL_loadImage(fileName, rect);
        struct TextCfg cfg;
        cfg.font = "external/kline/font/Deng.ttf";
        cfg.style = TTF_STYLE_NORMAL;
        cfg.color = { 255, 0, 0, 255 };
        cfg.rect = { { 0,0,300,60 }, {100,256,256,24} };
#ifndef _WIN32
        const char* text = "这ge是打在窗口上的等线字体";
        SDL_GL_showText(text, cfg);
#endif
    } else {
        qDebug(m_showSdl ? "SDL_GL_init fail!" : "No SDL GL");
    }
#endif
#endif
#endif // _GLVBO_
}

void QOglMaterial::resizeGL(int width, int height)
{
#ifdef _GLVBO_
    glViewport(0, 0, width, height);

    m_projection.setToIdentity();
    // 透视投影（height 为 0 时容错）
    qreal aspect = qreal(width) / qreal(height ? height : 1);
    m_projection.perspective(60.0f, aspect, 1.0f, 100.0f);
    // 沿 -Z 偏移，使物体进入可视范围
    m_projection.translate(0.0f, 0.0f, -2.0f);
#else
    if (height == 0) {
        height = 1;
    }
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspectRatio = (GLfloat)width / (GLfloat)height;
    GLdouble zNear = 0.1;
    GLdouble zFar = 100.0;

    GLdouble rFov = 45.0 * _PI_ / 180.0;
    glFrustum(-zNear * tan(rFov / 2.0) * aspectRatio,
        zNear * tan(rFov / 2.0) * aspectRatio,
        -zNear * tan(rFov / 2.0),
        zNear * tan(rFov / 2.0),
        zNear, zFar);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
#endif
}

void QOglMaterial::paintGL()
{
#ifdef  OGL_KVIEW_H_
    kv.InitGraph();
    kv.DrawCoord(0, 0);
    kv.GetMarkDatatoDraw("../KlineUtil/data/SH600747.DAT");
    qDebug() << "(" << kv.lastmarket.price << ")";
#else
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
#ifdef _GLVBO_
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, m_projection.data());

    glBindBuffer(GL_ARRAY_BUFFER, colorVbo);
    glEnableVertexAttribArray(clorLocation);
    glVertexAttribPointer(clorLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, verVbo);
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // 索引 buffer 无需启用顶点数组
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v_indexVbo);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(vertexLocation);
    glDisableVertexAttribArray(clorLocation);
#else
    // 先渲染滚动背景：正交投影全屏，纹理坐标偏移实现裁剪+前进效果
    if (mPng.isBgLoaded()) {
        mPng.showBackground(bgScrollU);
    }

    if (bingo) {
        textOut(30, 40, Qt::red, 3, "Helvetica");
        mPng.showFullPixels();
    } else {
        coord();
        textOut();

        glTranslatef(-1.0, 0, -8.0);
        glBegin(GL_QUADS);
        glVertex3f(0.0 + xVal, 0.4 + yVal, zZoom);
        glVertex3f(0.4 + xVal, 0.4 + yVal, zZoom);
        glVertex3f(0.4 + xVal, 0.0 + yVal, zZoom);
        glVertex3f(0.0 + xVal, 0.0 + yVal, zZoom);
        glEnd();

        glTranslatef(3, tHigh, -8.0);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);
        glVertex3f(0.0, 0.8, 0.0);
        glColor3f(0.0, 1.0, 0.0);
        glVertex3f(-0.8, -0.8, 0.0);
        glColor3f(0.0, 0.0, 1.0);
        glVertex3f(0.8, -0.8, 0.0);
        glEnd();
    }
#endif
#endif // OGL_KVIEW_H_
}
