#include "DeductionBoardWidget.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <GL/gl.h>

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════

DeductionBoardWidget::DeductionBoardWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setWindowTitle("声纹推演盘 - Deduction Board");
    setGeometry(100, 100, 1000, 680);
    setMinimumSize(820, 520);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // 初始化三个槽 + 关系
    m_slots.resize(3);
    m_relations = { Relation::Mask, Relation::Excite };

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DeductionBoardWidget::tick);
    m_timer->start(16);
    m_elapsed.start();

    loadChapter(0);
}

DeductionBoardWidget::~DeductionBoardWidget()
{
    if (m_timer && m_timer->isActive()) m_timer->stop();
}

void DeductionBoardWidget::loadChapter(int chapter)
{
    m_chapter = chapter;
    // 清空推演盘
    for (auto& s : m_slots) { s.sampleId = -1; s.action = Intervention::Play; s.anchor = TimeAnchor::Past; }
    m_hasResult = false;
    switch (chapter) {
    case 0: m_engine.setupPrologue(); break;
    case 1: m_engine.setupChapter1(); break;
    case 2: m_engine.setupChapter2(); break;
    case 3: m_engine.setupChapter3(); break;
    case 4: m_engine.setupFinale(); break;
    default: m_engine.setupPrologue(); break;
    }
}

// ═══════════════════════════════════════════════════════════════
//  OpenGL
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.04f, 0.04f, 0.07f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void DeductionBoardWidget::resizeGL(int w, int h)
{
    m_winW = w; m_winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = float(w) / float(h ? h : 1);
    if (aspect > 1.0f) glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
    else glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

void DeductionBoardWidget::tick()
{
    float dt = m_elapsed.elapsed() / 1000.0f;
    m_elapsed.restart();
    if (dt > 0.1f) dt = 0.1f;
    m_gameTime += dt;
    update();
}

void DeductionBoardWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    renderSamples();
    renderBoard();
    renderProposition();
    if (m_hasResult) renderResult();
    if (m_chapter == 4 && m_hasResult) renderPersonality();
}

// ═══════════════════════════════════════════════════════════════
//  样本库（左侧）
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::renderSamples()
{
    drawTextGL(-0.95f, 0.9f, "声纹样本库", QColor(0, 200, 255, 220), 0.7f);
    const auto& samples = m_engine.samples();
    float px = -0.95f, py = 0.75f, ih = 0.13f, gap = 0.03f;
    for (size_t i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i];
        float y = py - (ih + gap) * float(i);
        if (y < -0.85f) break;
        QColor bg = s.color(); bg.setAlpha(m_selectedSample == int(i) ? 70 : 30);
        drawRectGL(px, y - ih, 0.38f, ih, bg, true);
        QColor bc = s.color(); bc.setAlpha(m_selectedSample == int(i) ? 255 : 80);
        drawRectGL(px, y - ih, 0.38f, ih, bc, false);
        drawTextGL(px + 0.01f, y - 0.02f, s.name, s.color(), 0.4f);
        drawTextGL(px + 0.01f, y - 0.06f, "[" + s.kindLabel() + "] " + s.source, s.color().lighter(130), 0.28f);
        if (s.locked) drawTextGL(px + 0.28f, y - 0.06f, "🔒", QColor(255, 200, 100, 220), 0.35f);
        // 回声强化提示
        if (s.basePitch > 1.01f) drawTextGL(px + 0.01f, y - 0.09f, "▲音高+" + QString::number(s.basePitch, 'f', 1), QColor(255, 120, 120, 200), 0.25f);
    }
    // 锁存提示
    drawTextGL(-0.95f, -0.9f, "L: 锁存选中样本（防负片覆盖）", QColor(100, 130, 160, 140), 0.3f);
}

// ═══════════════════════════════════════════════════════════════
//  推演盘三槽（中部）
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::renderBoard()
{
    drawTextGL(-0.45f, 0.9f, "声纹推演盘", QColor(0, 220, 200, 230), 0.8f);
    static const char* slotNames[3] = { "样本", "干预动作", "时间锚点" };
    static const char* actions[] = { "播放", "删减", "变调", "伪造" };
    static const char* anchors[] = { "过去", "现在", "未来" };

    float bx = -0.45f, by = 0.7f, bw = 0.9f, bh = 0.18f, gap = 0.06f;
    for (int i = 0; i < 3; ++i) {
        float y = by - (bh + gap) * i;
        // 槽背景
        drawRectGL(bx, y - bh, bw, bh, QColor(15, 20, 35, 160), true);
        QColor bc = (m_dragging && m_selectedSample >= 0) ? QColor(0, 220, 200, 180) : QColor(60, 90, 130, 80);
        drawRectGL(bx, y - bh, bw, bh, bc, false);
        drawTextGL(bx + 0.02f, y - 0.03f, QString("槽%1：%2").arg(i + 1).arg(slotNames[i]), QColor(150, 180, 210, 170), 0.35f);

        auto& slot = m_slots[i];
        if (slot.sampleId >= 0) {
            auto* s = m_engine.findSample(slot.sampleId);
            if (s) {
                drawTextGL(bx + 0.02f, y - 0.08f, "→ " + s->name, s->color(), 0.45f);
                QString act = QString("动作：%1   锚点：%2").arg(actions[static_cast<int>(slot.action)]).arg(anchors[static_cast<int>(slot.anchor)]);
                drawTextGL(bx + 0.02f, y - 0.12f, act, QColor(180, 200, 220, 160), 0.32f);
            }
        } else {
            drawTextGL(bx + 0.02f, y - 0.08f, "（拖拽样本到此处）", QColor(80, 100, 130, 120), 0.32f);
        }
    }

    // 关系选择
    static const char* rels[] = { "掩盖", "激发", "反转", "同步" };
    drawTextGL(bx, by - 3 * (bh + gap) - 0.02f, "相对关系：", QColor(150, 180, 210, 170), 0.35f);
    for (int i = 0; i < 4; ++i) {
        float rx = bx + 0.35f + i * 0.15f;
        QColor rc = (m_selectedRelation == i) ? QColor(0, 220, 200, 255) : QColor(100, 140, 180, 140);
        drawTextGL(rx, by - 3 * (bh + gap) - 0.02f, QString("[%1]").arg(rels[i]), rc, 0.35f);
    }

    // 推演按钮提示
    float blink = 0.5f + 0.3f * sinf(m_gameTime * 3.0f);
    drawTextGL(bx, by - 3 * (bh + gap) - 0.2f, "ENTER: 执行推演  |  数字1-4切换关系  |  Tab: 切换动作/锚点", QColor(0, 220, 200, int(blink * 200)), 0.35f);
}

// ═══════════════════════════════════════════════════════════════
//  命题 + 心理变量滑块（顶部右侧）
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::renderProposition()
{
    Proposition* p = m_engine.currentProposition();
    if (p) {
        drawTextGL(0.5f, 0.9f, "待验证命题", QColor(255, 210, 100, 220), 0.6f);
        // 命题自动换行
        drawTextGL(0.5f, 0.8f, p->text, QColor(230, 210, 170, 200), 0.4f);
    }

    // 心理变量滑块（贪婪/恐惧）
    if (m_chapter >= 1) {
        drawTextGL(0.5f, 0.45f, "陈远山动机锚定偏差", QColor(180, 160, 200, 200), 0.45f);
        // 贪婪滑块
        drawTextGL(0.5f, 0.38f, QString("贪婪 %1%").arg(int(m_greed * 100)), QColor(255, 150, 80, 200), 0.35f);
        drawRectGL(0.5f, 0.32f, 0.5f, 0.03f, QColor(40, 40, 60, 180), true);
        drawRectGL(0.5f, 0.32f, 0.5f * m_greed, 0.03f, QColor(255, 150, 80, 220), true);
        // 恐惧滑块
        drawTextGL(0.5f, 0.24f, QString("恐惧 %1%").arg(int(m_fear * 100)), QColor(150, 160, 255, 200), 0.35f);
        drawRectGL(0.5f, 0.18f, 0.5f, 0.03f, QColor(40, 40, 60, 180), true);
        drawRectGL(0.5f, 0.18f, 0.5f * m_fear, 0.03f, QColor(150, 160, 255, 220), true);
    }
}

// ═══════════════════════════════════════════════════════════════
//  推演结果
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::renderResult()
{
    drawTextGL(-0.95f, -0.35f, "推演结果", QColor(0, 220, 160, 220), 0.55f);
    drawTextGL(-0.95f, -0.45f, m_lastResult.audioDescription, QColor(160, 220, 200, 180), 0.35f);
    drawTextGL(-0.95f, -0.58f, m_lastResult.logicInference, QColor(200, 200, 220, 180), 0.35f);
    drawTextGL(-0.95f, -0.68f, QString("心理可信度：%1%").arg(int(m_lastResult.credibility * 100)),
        m_lastResult.credibility > 0.65f ? QColor(255, 200, 80, 220) : QColor(150, 180, 200, 200), 0.4f);
    if (m_lastResult.rewritesMemory) {
        drawTextGL(-0.95f, -0.78f, "⚠ 此推演正在改写你的记忆库", QColor(255, 100, 100, 220), 0.4f);
    }
}

void DeductionBoardWidget::renderPersonality()
{
    PersonalityProfile pp = m_engine.generatePersonalityProfile();
    drawTextGL(0.5f, -0.5f, "推演板对你的侧写", QColor(255, 210, 100, 220), 0.5f);
    drawTextGL(0.5f, -0.6f, pp.summary, QColor(200, 190, 170, 180), 0.35f);
}

// ═══════════════════════════════════════════════════════════════
//  执行推演
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::performDeduction()
{
    // 过滤空槽
    std::vector<DeductionSlot> filled;
    for (const auto& s : m_slots) if (s.sampleId >= 0) filled.push_back(s);
    if (filled.size() < 2) {
        m_hasResult = true;
        m_lastResult.audioDescription = "需要至少两个声纹样本才能推演。";
        m_lastResult.logicInference = "推演板拒绝空转。";
        m_lastResult.credibility = 0.0f;
        m_lastResult.rewritesMemory = false;
        return;
    }

    // 记录锚点偏好
    for (const auto& s : filled) m_engine.recordAnchor(s.anchor);
    m_engine.setGreedFear(m_greed, m_fear);

    m_lastResult = m_engine.deduce(filled, m_relations);
    m_hasResult = true;

    // 合成音频描述
    std::vector<AudioLayer> layers;
    for (const auto& s : filled) {
        auto* smp = m_engine.findSample(s.sampleId);
        if (!smp) continue;
        AudioLayer l;
        l.frequency = 200.0f + (smp->id * 137) % 800;
        l.amplitude = 0.5f;
        l.durationSec = 0.8f;
        l.pitchScale = smp->basePitch;
        layers.push_back(l);
    }
    QString audioDesc = m_audio.synthesize(layers, false);
    m_lastResult.audioDescription += " | " + audioDesc;

    // 命题已推演
    Proposition* p = m_engine.currentProposition();
    if (p) m_engine.resolveProposition(p->id);
}

// ═══════════════════════════════════════════════════════════════
//  输入处理
// ═══════════════════════════════════════════════════════════════

QPointF DeductionBoardWidget::screenToGL(const QPoint& p) const
{
    float aspect = float(m_winW) / float(m_winH);
    float ghw, ghh;
    if (aspect > 1.0f) { ghw = aspect; ghh = 1.0f; } else { ghw = 1.0f; ghh = 1.0f / aspect; }
    float gx = (p.x() / float(m_winW)) * 2.0f * ghw - ghw;
    float gy = ghh - (p.y() / float(m_winH)) * 2.0f * ghh;
    return QPointF(gx, gy);
}

int DeductionBoardWidget::hitSample(const QPointF& gp) const
{
    const auto& samples = m_engine.samples();
    float px = -0.95f, py = 0.75f, ih = 0.13f, gap = 0.03f;
    for (size_t i = 0; i < samples.size(); ++i) {
        float y = py - (ih + gap) * float(i);
        if (gp.x() >= px && gp.x() <= px + 0.38f && gp.y() >= y - ih && gp.y() <= y) return int(i);
    }
    return -1;
}

int DeductionBoardWidget::hitSlot(const QPointF& gp) const
{
    float bx = -0.45f, by = 0.7f, bw = 0.9f, bh = 0.18f, gap = 0.06f;
    for (int i = 0; i < 3; ++i) {
        float y = by - (bh + gap) * i;
        if (gp.x() >= bx && gp.x() <= bx + bw && gp.y() >= y - bh && gp.y() <= y) return i;
    }
    return -1;
}

void DeductionBoardWidget::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        performDeduction();
        break;
    case Qt::Key_1: m_selectedRelation = 0; break;
    case Qt::Key_2: m_selectedRelation = 1; break;
    case Qt::Key_3: m_selectedRelation = 2; break;
    case Qt::Key_4: m_selectedRelation = 3; break;
    case Qt::Key_L:
        if (m_selectedSample >= 0 && m_selectedSample < int(m_engine.samples().size())) {
            m_engine.lockSample(m_engine.samples()[m_selectedSample].id);
        }
        break;
    case Qt::Key_Tab: {
        // 切换选中槽的动作/锚点
        for (int i = 0; i < 3; ++i) {
            if (m_slots[i].sampleId >= 0) {
                m_slots[i].action = static_cast<Intervention>((static_cast<int>(m_slots[i].action) + 1) % static_cast<int>(Intervention::Count));
            }
        }
        break;
    }
    case Qt::Key_Up: m_fear = std::min(1.0f, m_fear + 0.05f); break;
    case Qt::Key_Down: m_fear = std::max(0.0f, m_fear - 0.05f); break;
    case Qt::Key_Left: m_greed = std::max(0.0f, m_greed - 0.05f); break;
    case Qt::Key_Right: m_greed = std::min(1.0f, m_greed + 0.05f); break;
    }
    update();
}

void DeductionBoardWidget::mousePressEvent(QMouseEvent* e)
{
    QPointF gp = screenToGL(e->pos());
    if (e->button() == Qt::LeftButton) {
        int si = hitSample(gp);
        if (si >= 0) {
            m_selectedSample = si;
            m_dragging = true;
            m_dragPos = gp;
        }
        // 检查滑块
        if (m_chapter >= 1) {
            if (gp.x() >= 0.5f && gp.x() <= 1.0f && gp.y() >= 0.30f && gp.y() <= 0.36f) {
                m_draggingGreed = true; m_greed = (gp.x() - 0.5f) / 0.5f;
            }
            if (gp.x() >= 0.5f && gp.x() <= 1.0f && gp.y() >= 0.16f && gp.y() <= 0.22f) {
                m_draggingFear = true; m_fear = (gp.x() - 0.5f) / 0.5f;
            }
        }
    } else if (e->button() == Qt::RightButton) {
        // 右键清除槽
        int slotIdx = hitSlot(gp);
        if (slotIdx >= 0) { m_slots[slotIdx].sampleId = -1; }
        m_selectedSample = -1;
        m_dragging = false;
    }
}

void DeductionBoardWidget::mouseMoveEvent(QMouseEvent* e)
{
    QPointF gp = screenToGL(e->pos());
    float val = float(gp.x() - 0.5) / 0.5f;
    val = std::max(0.0f, std::min(1.0f, val));
    if (m_draggingGreed) m_greed = val;
    if (m_draggingFear) m_fear = val;
    if (m_dragging) m_dragPos = gp;
}

void DeductionBoardWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && m_dragging && m_selectedSample >= 0) {
        QPointF gp = screenToGL(e->pos());
        int slotIdx = hitSlot(gp);
        if (slotIdx >= 0) {
            m_slots[slotIdx].sampleId = m_engine.samples()[m_selectedSample].id;
            // 拖入槽后自动应用当前关系作为该槽的锚点
            m_slots[slotIdx].anchor = static_cast<TimeAnchor>(slotIdx % 3);
        }
        m_dragging = false;
        m_selectedSample = -1;
    }
    m_draggingGreed = false;
    m_draggingFear = false;
}

void DeductionBoardWidget::wheelEvent(QWheelEvent* e)
{
    (void)e;
}

// ═══════════════════════════════════════════════════════════════
//  绘制辅助
// ═══════════════════════════════════════════════════════════════

void DeductionBoardWidget::drawTextGL(float x, float y, const QString& t, const QColor& c, float s)
{
    QPainter p(this);
    p.setPen(c);
    QFont f("Monospace", 10);
    f.setStyleHint(QFont::Monospace);
    p.setFont(f);
    float aspect = float(m_winW) / float(m_winH);
    float ghw, ghh;
    if (aspect > 1.0f) { ghw = aspect; ghh = 1.0f; } else { ghw = 1.0f; ghh = 1.0f / aspect; }
    int sx = int((x + ghw) / (2.0f * ghw) * m_winW);
    int sy = int((ghh - y) / (2.0f * ghh) * m_winH);
    p.save();
    p.translate(sx, sy);
    p.scale(s, s);
    QStringList lines = t.split('\n');
    int maxPx = int(m_winW * 0.9f);
    QStringList wrapped;
    for (const QString& rl : lines) {
        QString cur; cur.reserve(rl.size());
        for (const QChar& ch : rl) {
            cur.append(ch);
            if (p.fontMetrics().horizontalAdvance(cur) > maxPx && cur.size() > 1) {
                QChar last = cur.at(cur.size() - 1); cur.chop(1);
                wrapped.push_back(cur); cur = last;
            }
        }
        wrapped.push_back(cur);
    }
    int lineH = 14;
    for (int li = 0; li < wrapped.size(); ++li) p.drawText(0, li * lineH, wrapped[li]);
    p.restore();
    p.end();
}

void DeductionBoardWidget::drawRectGL(float x, float y, float w, float h, const QColor& c, bool filled)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    if (filled) {
        glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    } else {
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
        glEnd();
    }
}

void DeductionBoardWidget::drawCircleGL(float cx, float cy, float r, const QColor& c, int seg)
{
    glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = 2.0f * ECHO_PI * i / seg;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}
