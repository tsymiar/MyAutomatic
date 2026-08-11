#include "EchoMainWindow.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <GL/gl.h>

// ==================== 构造/析构 ====================

EchoMainWindow::EchoMainWindow(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setWindowTitle("余音回响 - Echo Resonance");
    setGeometry(100, 100, 960, 640);
    setMinimumSize(800, 500);
    setFocusPolicy(Qt::StrongFocus);

    m_engine = new EchoEngine(this);

    m_tickTimer = new QTimer(this);
    connect(m_tickTimer, &QTimer::timeout, this, &EchoMainWindow::gameTick);
    m_tickTimer->start(16);
    m_elapsed.start();
}

EchoMainWindow::~EchoMainWindow()
{
    if (m_tickTimer && m_tickTimer->isActive())
        m_tickTimer->stop();
}

// ==================== OpenGL ====================

void EchoMainWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EchoMainWindow::resizeGL(int w, int h)
{
    m_winWidth = w;
    m_winHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = float(w) / float(h ? h : 1);
    if (aspect > 1.0f)
        glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    else
        glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ==================== 游戏循环 ====================

void EchoMainWindow::gameTick()
{
    float dt = m_elapsed.elapsed() / 1000.0f;
    m_elapsed.restart();
    if (dt > 0.1f) dt = 0.1f;
    m_gameTime += dt;
    update();
}

// ==================== 渲染主入口 ====================

void EchoMainWindow::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    switch (m_state) {
        case GameState::Title:     renderTitle(); break;
        case GameState::NameInput: renderNameInput(); break;
        case GameState::MainMenu:  renderMainMenu(); break;
        case GameState::GameOver:  renderTitle(); break;
    }
}

// ==================== 标题画面 ====================

void EchoMainWindow::renderTitle()
{
    // 声波背景
    for (int i = 0; i < 5; ++i) {
        float r = 0.3f + i * 0.12f + 0.02f * sinf(m_gameTime * 1.5f + i);
        float alpha = 0.15f - i * 0.025f;
        drawCircleGL(0.0f, 0.1f, r, QColor(0, 150, 255, int(alpha * 255)), 64);
    }

    // 标题
    const QString title = "余音回响";
    float startX = -title.length() * 0.06f;
    for (int i = 0; i < title.length(); ++i) {
        float pulse = 1.0f + 0.05f * sinf(m_gameTime * 3.0f + i * 1.2f);
        float x = startX + i * 0.12f;
        float y = 0.15f + 0.01f * sinf(m_gameTime * 2.0f + i * 0.8f);
        int alpha = 200 + 55 * int(sinf(m_gameTime * 2.5f + i));
        drawTextGL(x, y, QString(title[i]), QColor(0, 200, 255, alpha), pulse * 1.8f);
    }

    drawTextGL(-0.3f, -0.08f, "Echo Resonance", QColor(100, 180, 220, 150), 0.9f);

    float lineAlpha = 0.3f + 0.15f * sinf(m_gameTime * 2.0f);
    drawLineGL(-0.4f, -0.15f, 0.4f, -0.15f, QColor(0, 180, 255, int(lineAlpha * 255)), 1.5f);

    if (sinf(m_gameTime * 3.0f) > -0.3f)
        drawTextGL(-0.25f, -0.35f, "按 ENTER 开始", QColor(180, 200, 220, 200), 0.7f);

    drawTextGL(-0.4f, -0.55f, "ESC: 退出", QColor(100, 120, 140, 120), 0.45f);
}

// ==================== 角色名输入 ====================

void EchoMainWindow::renderNameInput()
{
    for (int i = 0; i < 4; ++i) {
        float r = 0.25f + i * 0.15f + 0.02f * sinf(m_gameTime * 1.5f + i);
        drawCircleGL(0.0f, 0.05f, r, QColor(0, 150, 255, int((0.12f - i * 0.025f) * 255)), 64);
    }

    drawTextGL(-0.35f, 0.55f, "你的名字", QColor(0, 200, 255, 220), 1.2f);
    drawTextGL(-0.45f, 0.35f, "请输入你在游戏中的角色名", QColor(150, 180, 200, 180), 0.55f);

    drawRectGL(-0.35f, 0.05f, 0.7f, 0.1f, QColor(10, 20, 40, 180), true);
    drawRectGL(-0.35f, 0.05f, 0.7f, 0.1f, QColor(0, 180, 255, 80), false);

    QString displayName = m_playerName;
    if (int(m_gameTime * 2) % 2 == 0)
        displayName += "|";
    if (m_playerName.isEmpty())
        drawTextGL(-0.3f, 0.09f, "输入你的名字...", QColor(60, 90, 130, 100), 0.55f);
    else
        drawTextGL(-0.3f, 0.09f, displayName, QColor(0, 255, 200, 220), 0.6f);

    float blink = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);
    drawTextGL(-0.3f, -0.15f, "按 ENTER 确认", QColor(0, 200, 255, int(blink * 200)), 0.55f);
    drawTextGL(-0.2f, -0.3f, "ESC: 返回 | 留空默认\"小周\"", QColor(100, 120, 140, 120), 0.4f);
}

// ==================== 主菜单 ====================

void EchoMainWindow::renderMainMenu()
{
    QString name = m_playerName.isEmpty() ? "小周" : m_playerName;
    drawTextGL(-0.5f, 0.7f, name + "，欢迎来到声学实验室", QColor(0, 200, 255, 220), 0.9f);

    QStringList menuItems = {
        "1. 开始第一幕：实验室",
        "2. 读取存档（未实现）",
        "3. 设置（未实现）"
    };
    for (int i = 0; i < menuItems.size(); ++i)
        drawTextGL(-0.4f, 0.3f - i * 0.15f, menuItems[i], QColor(150, 200, 220, 200), 0.55f);

    drawTextGL(-0.3f, -0.4f, "按数字键选择 | ESC: 返回标题", QColor(100, 120, 140, 120), 0.4f);
    drawTextGL(-0.5f, -0.7f, m_engine->getMentorMessage(0),
               QColor(200, 200, 150, 80), 0.35f);
}

// ==================== 绘制辅助 ====================

void EchoMainWindow::drawTextGL(float x, float y, const QString& text, const QColor& color, float scale)
{
    QPainter painter(this);
    painter.setPen(color);
    QFont font("Monospace", 10);
    font.setStyleHint(QFont::Monospace);
    painter.setFont(font);

    float aspect = float(m_winWidth) / float(m_winHeight);
    float glHalfW, glHalfH;
    if (aspect > 1.0f) { glHalfW = aspect; glHalfH = 1.0f; }
    else               { glHalfW = 1.0f;  glHalfH = 1.0f / aspect; }

    int sx = int((x + glHalfW) / (2.0f * glHalfW) * m_winWidth);
    int sy = int((glHalfH - y) / (2.0f * glHalfH) * m_winHeight);

    painter.save();
    painter.translate(sx, sy);
    painter.scale(scale, scale);
    painter.drawText(0, 0, text);
    painter.restore();
    painter.end();
}

void EchoMainWindow::drawRectGL(float x, float y, float w, float h, const QColor& color, bool filled)
{
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    if (filled) {
        glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    } else {
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    }
}

void EchoMainWindow::drawLineGL(float x1, float y1, float x2, float y2, const QColor& color, float width)
{
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1); glVertex2f(x2, y2);
    glEnd();
    glLineWidth(1.0f);
}

void EchoMainWindow::drawCircleGL(float cx, float cy, float r, const QColor& color, int segments)
{
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * 3.1415926535f * i / segments;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

// ==================== 输入处理 ====================

void EchoMainWindow::keyPressEvent(QKeyEvent* e)
{
    switch (m_state) {
    case GameState::Title:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            m_playerName.clear();
            m_state = GameState::NameInput;
        } else if (e->key() == Qt::Key_Escape) {
            close();
        }
        break;

    case GameState::NameInput:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            m_engine->setPlayerName(m_playerName.trimmed());
            m_state = GameState::MainMenu;
        } else if (e->key() == Qt::Key_Escape) {
            m_playerName.clear();
            m_state = GameState::Title;
        } else if (e->key() == Qt::Key_Backspace) {
            if (!m_playerName.isEmpty()) m_playerName.chop(1);
        } else if (!e->text().isEmpty() && e->text().at(0).isPrint()) {
            if (m_playerName.length() < 12) m_playerName += e->text();
        }
        break;

    case GameState::MainMenu:
        if (e->key() == Qt::Key_Escape) {
            m_state = GameState::Title;
        }
        // 数字键预留扩展点
        break;

    case GameState::GameOver:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            m_state = GameState::Title;
        }
        break;
    }
    update();
}

void EchoMainWindow::mousePressEvent(QMouseEvent* /*e*/)
{
    // 预留鼠标交互扩展点
}
