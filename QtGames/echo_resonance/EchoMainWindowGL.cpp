#include "EchoMainWindow.h"
#include "DeductionBoardWidget.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <cstdlib>
#include <GL/gl.h>
#include <GL/glu.h>

// ═══════════════════════════════════════════
//  OpenGL 初始化与图元绘制辅助
// ═══════════════════════════════════════════

// ═══════════════════════════════════════════
//  OpenGL 初始化
// ═══════════════════════════════════════════

void EchoMainWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void EchoMainWindow::resizeGL(int w, int h)
{
    m_winWidth = w; m_winHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    float aspect = float(w) / float(h ? h : 1);
    if (aspect > 1.0f) glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    else glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ═══════════════════════════════════════════
//  绘制辅助
// ═══════════════════════════════════════════

void EchoMainWindow::drawTextGL(float x, float y, const QString& t, const QColor& c, float s)
{
    QPainter p(this); p.setPen(c);
    QFont f("Monospace", 10); f.setStyleHint(QFont::Monospace); p.setFont(f);
    float asp = float(m_winWidth) / float(m_winHeight), ghw, ghh;
    if (asp > 1.0f) { ghw = asp;ghh = 1.0f; } else { ghw = 1.0f;ghh = 1.0f / asp; }
    int sx = int((x + ghw) / (2.0f * ghw) * m_winWidth), sy = int((ghh - y) / (2.0f * ghh) * m_winHeight);
    p.save(); p.translate(sx, sy); p.scale(s, s);
    // 支持多行：逐行绘制，行高约 14px（相对未缩放字体）
    QStringList rawLines = t.split('\n');
    QStringList lines;
    // 对每个原始行做自动折行：按最大像素宽度（窗口宽的 95%）折行，避免文字溢出屏幕
    int maxPx = int(m_winWidth * 0.95f);
    for (const QString& rl : rawLines) {
        QString cur;
        cur.reserve(rl.size());
        for (const QChar& ch : rl) {
            // 用 append 单字符避免 O(n²) 的字符串拷贝
            cur.append(ch);
            if (p.fontMetrics().horizontalAdvance(cur) > maxPx && cur.size() > 1) {
                // 回退一个字符到下一行
                QChar last = cur.at(cur.size() - 1);
                cur.chop(1);
                lines.push_back(cur);
                cur = last;
            }
        }
        lines.push_back(cur);
    }
    int lineH = int(14.0f);  // Monospace 10pt 行高
    for (int li = 0; li < lines.size(); ++li) {
        p.drawText(0, li * lineH, lines[li]);
    }
    p.restore(); p.end();
}

void EchoMainWindow::drawRectGL(float x, float y, float w, float h, const QColor& c, bool f)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    if (f) { glBegin(GL_QUADS);glVertex2f(x, y);glVertex2f(x + w, y);glVertex2f(x + w, y + h);glVertex2f(x, y + h);glEnd(); } else { glBegin(GL_LINE_LOOP);glVertex2f(x, y);glVertex2f(x + w, y);glVertex2f(x + w, y + h);glVertex2f(x, y + h);glEnd(); }
}

void EchoMainWindow::drawLineGL(float x1, float y1, float x2, float y2, const QColor& c, float w)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF()); glLineWidth(w);
    glBegin(GL_LINES);glVertex2f(x1, y1);glVertex2f(x2, y2);glEnd(); glLineWidth(1.0f);
}

void EchoMainWindow::drawCircleGL(float cx, float cy, float r, const QColor& c, int seg)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF()); glBegin(GL_TRIANGLE_FAN);glVertex2f(cx, cy);
    for (int i = 0;i <= seg;++i) { float a = 2.0f * ECHO_PI * i / seg;glVertex2f(cx + r * cosf(a), cy + r * sinf(a)); }glEnd();
}
