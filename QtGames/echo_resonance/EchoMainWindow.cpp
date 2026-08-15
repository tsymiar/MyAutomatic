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
//  构造/析构
// ═══════════════════════════════════════════

EchoMainWindow::EchoMainWindow(QWidget* parent) : QOpenGLWidget(parent)
{
    setWindowTitle("余音回响 - Echo Resonance");
    setGeometry(100, 100, 960, 640);
    setMinimumSize(800, 500);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_engine = new EchoEngine(this);
    connect(m_engine, &EchoEngine::fragmentPlaced, this, &EchoMainWindow::onFragmentPlaced);
    connect(m_engine, &EchoEngine::fragmentRemoved, this, &EchoMainWindow::onFragmentRemoved);
    connect(m_engine, &EchoEngine::sceneCompleted, this, &EchoMainWindow::onSceneCompleted);
    connect(m_engine, &EchoEngine::paranoiaChanged, this, &EchoMainWindow::onParanoiaChanged);
    connect(m_engine, &EchoEngine::anxietyChanged, this, &EchoMainWindow::onAnxietyChanged);
    connect(m_engine, &EchoEngine::hallucinationTriggered, this, &EchoMainWindow::onHallucinationTriggered);
    connect(m_engine, &EchoEngine::reverseUnlocked, this, &EchoMainWindow::onReverseUnlocked);
    connect(m_engine, &EchoEngine::noiseCommunion, this, &EchoMainWindow::onNoiseCommunion);
    connect(m_engine, &EchoEngine::choiceRequired, this, [this](const QString& prompt, const QStringList& options) {
        showChoice(prompt, options);
        });
    connect(m_engine, &EchoEngine::endingReached, this, [this](EndingType) {
        m_state = GameState::EndingScreen;
        });
    connect(m_engine, &EchoEngine::morseMessage, this, [this](const QString&) {
        // 摩斯电码消息由 renderMorseIndicator 定时轮询 getMorseMessage() 显示
        });
    connect(m_engine, &EchoEngine::silenceMode, this, [this](bool active) {
        m_silenceMode = active;
        });
    m_tickTimer = new QTimer(this);
    connect(m_tickTimer, &QTimer::timeout, this, &EchoMainWindow::gameTick);
    m_tickTimer->start(16);
    m_elapsed.start();
    for (int i = 0; i < 64; ++i) {
        SpectrumLine l; l.frequency = 20.0f * powf(2.0f, i / 8.0f);
        l.amplitude = (rand() % 100) / 200.0f; l.phase = (rand() % 628) / 100.0f;
        l.color = QColor(0, 180, 255, 60); m_spectrumLines.push_back(l);
    }
}

EchoMainWindow::~EchoMainWindow()
{
    if (m_tickTimer && m_tickTimer->isActive()) m_tickTimer->stop();
}

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
//  游戏循环
// ═══════════════════════════════════════════

void EchoMainWindow::gameTick()
{
    float dt = m_elapsed.elapsed() / 1000.0f; m_elapsed.restart();
    if (dt > 0.1f) dt = 0.1f;
    m_gameTime += dt;
    m_titlePulse = 1.0f + 0.08f * sinf(m_gameTime * 2.5f);
    m_titleWaveOffset += dt * 0.8f;
    if (m_glitchIntensity > 0.0f) { m_glitchIntensity -= dt * 0.3f;if (m_glitchIntensity < 0.0f)m_glitchIntensity = 0.0f; }
    m_noiseOffset += dt * 15.0f;
    m_bgFlicker = 0.02f * sinf(m_gameTime * 3.7f) + 0.01f * sinf(m_gameTime * 7.3f);
    float tp = m_engine->paranoiaLevel(); m_paranoiaDisplay += (tp - m_paranoiaDisplay) * dt * 3.0f;
    float ta = m_engine->anxietyLevel(); m_anxietyDisplay += (ta - m_anxietyDisplay) * dt * 3.0f;
    float tl = m_engine->lowFreqIntensity(); m_lowFreqDisplay += (tl - m_lowFreqDisplay) * dt * 2.0f;
    float tr = m_engine->resonanceLevel(); m_resonanceDisplay += (tr - m_resonanceDisplay) * dt * 3.0f;
    m_resonanceAuraPhase += dt * 1.5f;

    // ── 巧妙机制 3：更新碎片回声 ──
    m_engine->updateEchoes(dt);

    // ── 巧妙机制 4：静默计时（老刘的沉默）──
    // 在时间轴/碎片界面静默（无操作）时累积
    if (m_state == GameState::TimelinePuzzle || m_state == GameState::FragmentSelect) {
        m_idleTime += dt;
        m_engine->updateSilenceMorse(dt);
    } else {
        m_idleTime = 0.0f;
    }

    // ── 耳鸣指引（林薇潜意识引导）──
    // 在解谜场景停留超 2 分钟未推进时激活
    if (m_state == GameState::FragmentSelect || m_state == GameState::TimelinePuzzle || m_state == GameState::Tutorial) {
        m_stuckTime += dt;
        if (m_stuckTime >= 120.0f && !m_tinnitusActive) {
            m_tinnitusActive = true;
        }
    } else {
        m_stuckTime = 0.0f;
        m_tinnitusActive = false;
    }
    if (m_tinnitusActive) m_tinnitusPhase += dt;
    // 静默模式衰减
    if (m_silenceMode && m_silenceAlpha < 0.9f) m_silenceAlpha += dt * 2.0f;
    if (!m_silenceMode && m_silenceAlpha > 0.0f) m_silenceAlpha -= dt * 1.5f;
    if (m_silenceAlpha < 0.0f)m_silenceAlpha = 0.0f;
    for (auto& l : m_spectrumLines) { l.phase += dt * l.frequency * 0.1f; float ta = (rand() % 100) / 200.0f; l.amplitude += (ta - l.amplitude) * dt * 2.0f; }
    for (auto it = m_glitchParticles.begin();it != m_glitchParticles.end();) {
        it->x += it->vx * dt;it->y += it->vy * dt;it->life -= dt;
        if (it->life <= 0.0f) it = m_glitchParticles.erase(it); else ++it;
    }
    if (m_engine->isDistorted() && m_state == GameState::TimelinePuzzle) {
        m_glitchIntensity = std::min(1.0f, m_glitchIntensity + dt * 0.5f);
        if (rand() % 100 < 40) { GlitchParticle p;p.x = (rand() % 200 - 100) / 100.0f;p.y = (rand() % 200 - 100) / 100.0f;p.vx = (rand() % 100 - 50) / 200.0f;p.vy = (rand() % 100 - 50) / 200.0f;p.life = 0.5f + (rand() % 50) / 100.0f;p.maxLife = p.life;p.color = QColor(255, rand() % 100, rand() % 50, 100);m_glitchParticles.push_back(p); }
    }
    for (auto it = m_hallucinations.begin();it != m_hallucinations.end();) {
        it->x += it->speedX * dt;it->y += it->speedY * dt;it->life -= dt;it->alpha = std::min(it->alpha, it->life / 3.0f);
        if (it->life <= 0.0f || it->alpha <= 0.0f) it = m_hallucinations.erase(it); else ++it;
    }
    for (auto it = m_noiseTexts.begin();it != m_noiseTexts.end();) {
        it->life -= dt;it->alpha = std::min(it->alpha, it->life / 2.0f);
        if (it->life <= 0.0f || it->alpha <= 0.0f) it = m_noiseTexts.erase(it); else ++it;
    }
    if (m_state == GameState::MentorMessage && m_mentorMessageAlpha < 1.0f) { m_mentorMessageAlpha += dt * 0.5f;if (m_mentorMessageAlpha > 1.0f)m_mentorMessageAlpha = 1.0f; }
    if (m_state == GameState::ChapterIntro && m_chapterIntroAlpha < 1.0f) { m_chapterIntroAlpha += dt * 0.8f;if (m_chapterIntroAlpha > 1.0f)m_chapterIntroAlpha = 1.0f; }
    // 结局余波：淡入 + 计时
    if (m_state == GameState::EpilogueScreen) {
        if (m_epilogueAlpha < 1.0f) { m_epilogueAlpha += dt * 0.4f;if (m_epilogueAlpha > 1.0f)m_epilogueAlpha = 1.0f; }
        if (m_epilogueAlpha >= 1.0f) { m_epilogueTimer += dt;if (m_epilogueTimer >= 90.0f)m_epilogueFinished = true; }
    }
    update();
}

// ═══════════════════════════════════════════
//  渲染主入口
// ═══════════════════════════════════════════

void EchoMainWindow::paintGL()
{
    float bgR = 0.05f + m_bgFlicker, bgG = 0.05f + m_bgFlicker * 0.5f, bgB = 0.08f + m_bgFlicker;
    glClearColor(bgR, bgG, bgB, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    renderSpectrumVisualization();
    if (m_glitchIntensity > 0.01f) renderGlitchEffects();
    if (m_resonanceDisplay > 0.05f) renderResonanceAura();
    renderFragmentEchoes();
    if (m_engine->isRightEarSilenced()) renderSilencePunishment();
    if (m_engine->isSilenceMorseReady()) renderSilenceMorse();
    renderHallucinations();
    renderNoiseDialogues();
    switch (m_state) {
    case GameState::Title: renderTitle(); break;
    case GameState::NameInput: renderNameInput(); break;
    case GameState::Tutorial: renderTutorial(); break;
    case GameState::FragmentSelect: renderFragmentSelect(); break;
    case GameState::TimelinePuzzle: renderTimelinePuzzle(); break;
    case GameState::SceneComplete: renderSceneComplete(); break;
    case GameState::MentorMessage: renderMentorMessage(); break;
    case GameState::PlayerChoice: renderPlayerChoice(); break;
    case GameState::ChapterIntro: renderChapterIntro(); break;
    case GameState::EchoArchive: renderEchoArchive(); break;
    case GameState::EndingScreen: renderEndingScreen(); break;
    case GameState::EpilogueScreen: renderEpilogueScreen(); break;
    case GameState::GameOver: renderTitle(); break;
    }
    renderReverseIndicator();
    renderMorseIndicator();
    renderParanoiaMeter();
    renderAnxietyMeter();
    if (m_silenceAlpha > 0.01f) renderSilenceOverlay();
    if (m_tinnitusActive) renderTinnitusGuide();
}

// ═══════════════════════════════════════════
//  标题画面
// ═══════════════════════════════════════════

void EchoMainWindow::renderTitle()
{
    for (int i = 0;i < 5;++i) { float r = 0.3f + i * 0.12f + 0.02f * sinf(m_gameTime * 1.5f + i), a = 0.15f - i * 0.025f;drawCircleGL(0, 0.1f, r, QColor(0, 150, 255, int(a * 255)), 64); }
    const QString title = "余音回响"; float tw = title.length() * 0.12f, sx = -tw / 2.0f;
    for (int i = 0;i < title.length();++i) { float p = 1.0f + 0.05f * sinf(m_gameTime * 3.0f + i * 1.2f), x = sx + i * 0.12f, y = 0.15f + 0.01f * sinf(m_gameTime * 2.0f + i * 0.8f);int a = 200 + 55 * sinf(m_gameTime * 2.5f + i);drawTextGL(x, y, QString(title[i]), QColor(0, 200, 255, a), p * 1.8f); }
    drawTextGL(-0.3f, -0.08f, "Echo Resonance", QColor(100, 180, 220, 150), 0.9f);
    float ly = -0.15f, la = 0.3f + 0.15f * sinf(m_gameTime * 2.0f);drawLineGL(-0.4f, ly, 0.4f, ly, QColor(0, 180, 255, int(la * 255)), 1.5f);
    if (sinf(m_gameTime * 3.0f) > -0.3f) drawTextGL(-0.25f, -0.35f, "按 ENTER 开始", QColor(180, 200, 220, 200), 0.7f);
    drawTextGL(-0.4f, -0.55f, "按 D 进入声纹推演盘", QColor(0, 220, 200, 200), 0.5f);
    drawTextGL(-0.4f, -0.68f, "操作：鼠标拖拽碎片到时间轴 | ESC 退出 | R 重置", QColor(100, 120, 140, 120), 0.45f);
    for (int i = 0;i < 200;++i) {
        float t = i / 200.0f, x = -0.95f + t * 1.9f, y = -0.85f + 0.03f * sinf(t * 30.0f + m_gameTime * 4.0f) * (1.0f + 0.5f * sinf(m_gameTime));
        if (i > 0) { float t2 = (i - 1) / 200.0f, x2 = -0.95f + t2 * 1.9f, y2 = -0.85f + 0.03f * sinf(t2 * 30.0f + m_gameTime * 4.0f) * (1.0f + 0.5f * sinf(m_gameTime));drawLineGL(x2, y2, x, y, QColor(255, 40, 40, 30), 0.5f); }
    }
}

// ═══════════════════════════════════════════
//  角色名输入
// ═══════════════════════════════════════════

void EchoMainWindow::renderNameInput()
{
    for (int i = 0;i < 4;++i) { float r = 0.25f + i * 0.15f + 0.02f * sinf(m_gameTime * 1.5f + i), a = 0.12f - i * 0.025f;drawCircleGL(0, 0.05f, r, QColor(0, 150, 255, int(a * 255)), 64); }
    drawTextGL(-0.35f, 0.55f, "你的名字", QColor(0, 200, 255, 220), 1.2f);
    drawTextGL(-0.45f, 0.35f, "请输入你在游戏中的角色名", QColor(150, 180, 200, 180), 0.55f);
    drawTextGL(-0.55f, 0.25f, "导师会用这个名字称呼你，这会影响你看到的叙事内容", QColor(100, 130, 160, 150), 0.4f);
    drawRectGL(-0.35f, 0.05f, 0.7f, 0.1f, QColor(10, 20, 40, 180), true);
    drawRectGL(-0.35f, 0.05f, 0.7f, 0.1f, QColor(0, 180, 255, 80), false);
    QString dn = m_playerName;
    if (int(m_gameTime * 2) % 2 == 0) dn += "|";
    if (dn.isEmpty() || (dn == "|" && m_playerName.isEmpty())) drawTextGL(-0.3f, 0.09f, "输入你的名字...", QColor(60, 90, 130, 100), 0.55f);
    else drawTextGL(-0.3f, 0.09f, dn, QColor(0, 255, 200, 220), 0.6f);
    float b = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);
    drawTextGL(-0.3f, -0.15f, "按 ENTER 确认", QColor(0, 200, 255, int(b * 200)), 0.55f);
    drawTextGL(-0.2f, -0.3f, QString("ESC: 返回标题 | 留空则使用默认名\"%1\"").arg(DEFAULT_PLAYER_NAME), QColor(100, 120, 140, 120), 0.4f);
    for (int i = 0;i < 200;++i) {
        float t = i / 200.0f, x = -0.95f + t * 1.9f, y = -0.85f + 0.02f * sinf(t * 25.0f + m_gameTime * 3.0f);
        if (i > 0) { float t2 = (i - 1) / 200.0f, x2 = -0.95f + t2 * 1.9f, y2 = -0.85f + 0.02f * sinf(t2 * 25.0f + m_gameTime * 3.0f);drawLineGL(x2, y2, x, y, QColor(255, 40, 40, 20), 0.3f); }
    }
}

// ═══════════════════════════════════════════
//  教程界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderTutorial()
{
    drawTextGL(-0.9f, 0.85f, "序章：入职第7天 — 教程", QColor(0, 200, 255, 220), 1.0f);
    drawTextGL(-0.9f, 0.72f, "目标：还原昨天下午茶水间的对话", QColor(150, 200, 220, 180), 0.5f);
    drawTextGL(-0.9f, 0.65f, "操作：点击左侧碎片 → 拖放到时间轴槽位 → 按 TAB 切换到时间轴", QColor(120, 160, 200, 180), 0.4f);
    drawTextGL(-0.9f, 0.58f, "蓝色碎片=真实残留 | 青色=可疑 | 黄色=噪声 | 红色=伪造", QColor(100, 140, 180, 150), 0.38f);
    // 锚点提示
    AnchorType a = m_engine->currentAnchor();
    QString an = "锚点音：";
    switch (a) {
    case AnchorType::DoorBeep:an += "门禁刷卡声（必须放在第1个槽位）";break;
    case AnchorType::CoffeeMachine:an += "咖啡机启动声";break;
    case AnchorType::HeartMonitor:an += "心率监护仪";break;
    default:an += "无";break;
    }
    drawTextGL(-0.9f, -0.85f, an, QColor(255, 200, 0, 180), 0.4f);
    drawTextGL(0.1f, -0.85f, "按 TAB 开始拼接 | ESC 退出", QColor(100, 120, 140, 120), 0.4f);
}

// ═══════════════════════════════════════════
//  章节介绍
// ═══════════════════════════════════════════

void EchoMainWindow::renderChapterIntro()
{
    float a = m_chapterIntroAlpha;
    glClearColor(0.02f * a, 0.02f * a, 0.04f * a, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    drawTextGL(-0.6f, 0.5f, m_engine->chapterName(), QColor(0, 200, 255, int(a * 220)), 1.3f);
    QString desc = m_engine->chapterDescription();
    drawTextGL(-0.8f, 0.2f, desc, QColor(180, 200, 220, int(a * 180)), 0.42f);
    if (a > 0.8f) { float b = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);drawTextGL(-0.3f, -0.5f, "按 ENTER 继续", QColor(0, 200, 255, int(b * 200)), 0.6f); }
}

void EchoMainWindow::showChapterIntro()
{
    m_prevState = m_state;
    m_chapterIntroAlpha = 0.0f;
    m_chapterIntroShown = false;
    m_state = GameState::ChapterIntro;
    // 章节推进时揭示对应角色（接入 revealCharacter）
    // 0=周宁 1=林薇 2=陈远山 3=老刘 4=何悦 5=底噪
    GameChapter ch = m_engine->currentChapter();
    switch (ch) {
    case GameChapter::Prologue: m_engine->revealCharacter(0); m_engine->revealCharacter(1); break; // 周宁+林薇
    case GameChapter::Chapter2: m_engine->revealCharacter(2); m_engine->revealCharacter(3); break; // 陈远山+老刘
    case GameChapter::Chapter3: m_engine->revealCharacter(4); break; // 何悦
    case GameChapter::Chapter4: m_engine->revealCharacter(5); break; // 底噪
    default: break;
    }
}

// ═══════════════════════════════════════════
//  碎片选择界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderFragmentSelect()
{
    drawTextGL(-0.9f, 0.85f, m_engine->chapterName(), QColor(0, 200, 255, 220), 1.1f);
    // 章节描述直接使用引擎返回的 \n 换行，不再手动切行
    QString desc = m_engine->chapterDescription();
    drawTextGL(-0.9f, 0.72f, desc, QColor(150, 180, 200, 180), 0.4f);
    // 碎片列表起始位置根据描述行数动态下移，避免重叠
    int descLines = desc.count('\n') + 1;
    float py = 0.72f - descLines * 0.04f - 0.15f;
    float px = -0.9f, pw = 0.85f, ih = 0.11f, gap = 0.02f;
    auto& frags = m_engine->fragments();
    int tf = static_cast<int>(frags.size());
    float so = m_fragmentScrollY;
    int vs = std::max(0, int(-so / (ih + gap))), ve = std::min(tf, vs + 12);
    for (int i = vs;i < ve;++i) {
        auto& f = frags[i]; if (f.isPlaced) continue;
        float y = py + (ih + gap) * i + so; if (y < -1.0f || y>1.0f) continue;
        QColor bc = f.displayColor(); bc.setAlpha(40); drawRectGL(px, y - ih, pw, ih, bc, true);
        bool sel = (m_selectedFragmentIndex == i);
        QColor boc = f.displayColor(); boc.setAlpha(sel ? 255 : 80); drawRectGL(px, y - ih, pw, ih, boc, false);
        drawTextGL(px + 0.02f, y - 0.02f, f.name, f.displayColor(), 0.55f);
        drawTextGL(px + 0.02f, y - 0.06f, f.credibilityLabel(), f.displayColor().lighter(130), 0.35f);
        if (f.isAnchor) { drawTextGL(px + 0.02f, y - 0.09f, "[锚点音]", QColor(255, 200, 0, 180), 0.3f); }
        // 何悦照妖镜：被照出原形的伪造碎片（接入 isCredibilityExposed）
        if (m_engine->isCredibilityExposed(f)) {
            drawTextGL(px + pw - 0.30f, y - 0.08f, "[被照出]", QColor(0, 255, 200, 220), 0.3f);
        }
        QString d = QString("%1秒").arg(f.durationSec, 0, 'f', 1);
        drawTextGL(px + pw - 0.12f, y - 0.04f, d, QColor(150, 150, 150, 150), 0.35f);
    }
    QString cs = QString("可用碎片: %1 / %2 (已放置: %3)").arg(tf - m_engine->placedFragmentCount()).arg(tf).arg(m_engine->placedFragmentCount());
    drawTextGL(-0.9f, -0.9f, cs, QColor(120, 140, 160, 150), 0.4f);
    if (m_selectedFragmentIndex >= 0 && m_selectedFragmentIndex < tf) {
        auto& f = frags[m_selectedFragmentIndex];
        drawTextGL(0.1f, 0.55f, "已选中:", QColor(0, 200, 255, 200), 0.6f);
        drawTextGL(0.1f, 0.48f, f.name, f.displayColor(), 0.7f);
        drawTextGL(0.1f, 0.42f, f.description, QColor(150, 180, 200, 180), 0.35f);
        drawTextGL(0.1f, 0.36f, QString("可信度: %1").arg(f.credibilityLabel()), f.displayColor(), 0.4f);
        if (f.isReversed) {
            drawTextGL(0.1f, 0.30f, "[已逆向]", QColor(255, 100, 200, 200), 0.4f);
            // 逆向揭示的秘密（接入 getReverseDescription）
            QString rd = m_engine->getReverseDescription(f.id);
            if (!rd.isEmpty()) drawTextGL(0.1f, 0.24f, "逆向后揭示: " + rd, QColor(255, 150, 220, 200), 0.35f);
        }
    }
    drawTextGL(0.1f, -0.6f, "ESC: 返回 | TAB: 时间轴 | R: 逆向播放 | E: 回声档案", QColor(100, 120, 140, 120), 0.4f);
    // 低频警告（接入 getLowFreqWarning）
    QString lfw = m_engine->getLowFreqWarning();
    if (!lfw.isEmpty()) {
        float a = 0.4f + 0.25f * sinf(m_gameTime * 4.0f);
        drawTextGL(0.1f, -0.75f, lfw, QColor(255, 80, 80, int(a * 255)), 0.4f);
    }
}

// ═══════════════════════════════════════════
//  时间轴拼图
// ═══════════════════════════════════════════

void EchoMainWindow::renderTimelinePuzzle()
{
    drawTextGL(-0.9f, 0.85f, m_engine->chapterName(), QColor(0, 200, 255, 220), 0.9f);
    float coh = m_engine->calculateCoherence();
    QString cs = QString("场景一致性: %1%").arg(int(coh * 100));
    QColor cc = coh > 0.7f ? QColor(0, 200, 100) : (coh > 0.4f ? QColor(200, 200, 0) : QColor(255, 60, 60));
    drawTextGL(0.2f, 0.85f, cs, cc, 0.55f);
    if (m_engine->isDistorted()) { float a = 0.5f + 0.3f * sinf(m_gameTime * 6.0f);drawTextGL(0.2f, 0.78f, "⚠ 声纹冲突 - 场景已扭曲", QColor(255, 80, 40, int(a * 255)), 0.5f); }
    auto& frags = m_engine->fragments();
    float pnx = -0.95f, pny = 0.65f, iw = 0.25f, ih = 0.06f, gap = 0.01f; int col = 0, row = 0, mc = 3;
    for (size_t i = 0;i < frags.size();++i) {
        auto& f = frags[i]; if (f.isPlaced) continue;
        float x = pnx + col * (iw + gap), y = pny - row * (ih + gap * 3);
        QColor bc = f.displayColor(); bc.setAlpha(m_selectedFragmentIndex == int(i) ? 60 : 25); drawRectGL(x, y - ih, iw, ih, bc, true);
        QColor boc = f.displayColor(); boc.setAlpha(m_selectedFragmentIndex == int(i) ? 255 : 60); drawRectGL(x, y - ih, iw, ih, boc, false);
        drawTextGL(x + 0.01f, y - 0.02f, f.name, f.displayColor(), 0.4f);
        drawTextGL(x + 0.01f, y - 0.05f, f.credibilityLabel(), f.displayColor().lighter(130), 0.28f);
        col++; if (col >= mc) { col = 0;row++; }
    }
    float tlx = TIMELINE_X, tly = TIMELINE_Y, tlw = TIMELINE_W, tlh = TIMELINE_H;
    int sc = m_engine->timelineSlotCount();
    float sw = std::min(0.1f, (tlw - (sc - 1) * SLOT_GAP) / sc);
    float tsw = sc * sw + (sc - 1) * SLOT_GAP, ssx = tlx + (tlw - tsw) / 2.0f;
    drawRectGL(tlx, tly, tlw, tlh, QColor(10, 15, 25, 180), true);
    drawRectGL(tlx, tly, tlw, tlh, QColor(0, 150, 200, 60), false);
    drawTextGL(tlx, tly + tlh + 0.02f, "时间轴 (拖放碎片到槽位)", QColor(100, 160, 200, 180), 0.35f);
    for (int i = 0;i < sc;++i) {
        float sx = ssx + i * (sw + SLOT_GAP), sy = tly + 0.02f, sh = tlh - 0.04f;
        int fid = m_engine->timeline()[i].fragmentId;
        if (fid >= 0) { for (auto& f : frags)if (f.id == fid) { QColor sc = f.displayColor();sc.setAlpha(80);drawRectGL(sx, sy, sw, sh, sc, true);drawRectGL(sx, sy, sw, sh, f.displayColor(), false);drawTextGL(sx + 0.005f, sy + sh / 2 + 0.01f, f.name, f.displayColor(), 0.3f);break; } } else {
            drawRectGL(sx, sy, sw, sh, QColor(30, 40, 60, 60), true);drawRectGL(sx, sy, sw, sh, QColor(60, 80, 120, 40), false);
            QString label = QString::number(i + 1);
            if (m_engine->timeline()[i].isAnchorSlot) label = "[锚]" + label;
            drawTextGL(sx + 0.005f, sy + sh / 2 + 0.01f, label, QColor(60, 80, 120, 80), 0.25f);
        }
    }
    if (m_isDragging && m_selectedFragmentIndex >= 0) {
        auto& f = frags[m_selectedFragmentIndex];
        QPointF gp = screenToGL(QPoint(int(m_dragCurrentPos.x()), int(m_dragCurrentPos.y())));
        QColor dc = f.displayColor();dc.setAlpha(120);drawRectGL(gp.x() - 0.04f, gp.y() - 0.03f, 0.08f, 0.06f, dc, true);drawTextGL(gp.x() - 0.03f, gp.y(), f.name, f.displayColor(), 0.35f);
    }
    // 锚点状态提示（接入 isAnchorCorrect）
    if (!m_engine->isAnchorCorrect()) {
        float a = 0.5f + 0.3f * sinf(m_gameTime * 5.0f);
        drawTextGL(0.1f, -0.9f, "⚠ 锚点音未正确放置", QColor(255, 180, 40, int(a * 255)), 0.4f);
    }
    drawTextGL(-0.9f, -0.9f, "ESC: 返回碎片选择 | R: 重置 | 右键槽位移除 | TAB: 逆向播放选中", QColor(100, 120, 140, 120), 0.35f);
}

// ═══════════════════════════════════════════
//  场景完成
// ═══════════════════════════════════════════

void EchoMainWindow::renderSceneComplete()
{
    bool d = m_engine->isDistorted();
    if (d) {
        glClearColor(0.12f, 0.03f, 0.03f, 1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        for (int i = 0;i < 200;++i) { float x = (rand() % 200 - 100) / 100.0f, y = (rand() % 200 - 100) / 100.0f, s = (rand() % 5) / 500.0f;drawRectGL(x, y, s, s, QColor(255, rand() % 60, rand() % 60, rand() % 60), true); }
    } else { for (int i = 0;i < 3;++i) { float r = 0.4f + i * 0.15f, a = 0.08f - i * 0.02f;drawCircleGL(0, 0.2f, r, QColor(0, 150, 255, int(a * 255)), 64); } }
    QString tt = d ? "场景重建 - 扭曲版本" : "场景重建完成";
    drawTextGL(-0.4f, 0.5f, tt, d ? QColor(255, 80, 40) : QColor(0, 200, 255), 1.2f);
    float coh = m_engine->calculateCoherence();
    drawTextGL(-0.3f, 0.3f, QString("声纹一致性: %1%").arg(int(coh * 100)), d ? QColor(255, 120, 60) : QColor(0, 200, 150), 0.7f);
    QString desc = d ? "你使用了冲突的声纹碎片强行拼接。\n现实出现了裂痕——但裂痕中，有时会透出真相。" : "音频场景完整重现。但请记住导师的警告：别相信你听到的任何声音。";
    drawTextGL(-0.5f, 0.1f, desc, QColor(180, 200, 220, 200), 0.45f);
    if (d || m_engine->paranoiaLevel() > 0.5f) { float a = 0.5f + 0.3f * sinf(m_gameTime * 2.0f);QString msg = m_engine->getMentorMessage(m_mentorMessageIndex);drawTextGL(-0.5f, -0.2f, "\"" + msg + "\"", QColor(255, 200, 100, int(a * 255)), 0.55f); }
    // 角色对话（接入 getCharacterDialogue，扭曲版本下浮现角色内心）
    if (d) {
        QString speaker;
        GameChapter ch = m_engine->currentChapter();
        if (ch == GameChapter::Chapter2) speaker = "陈远山";
        else if (ch == GameChapter::Chapter3) speaker = "何悦";
        else if (ch == GameChapter::Chapter4) speaker = "老刘";
        if (!speaker.isEmpty()) {
            QString cd = m_engine->getCharacterDialogue(speaker, 0);
            if (!cd.isEmpty()) drawTextGL(-0.5f, -0.8f, "[" + speaker + "] " + cd, QColor(150, 200, 255, 160), 0.38f);
        }
    }
    drawTextGL(-0.3f, -0.5f, "ENTER: 继续 | R: 重新拼接", QColor(150, 180, 200, 180), 0.5f);
    if (d) { float b = sinf(m_gameTime * 4.0f) > 0 ? 1.0f : 0.3f;drawTextGL(-0.2f, -0.6f, "提示：扭曲版本可能揭示了隐藏线索", QColor(255, 80, 40, int(b * 200)), 0.4f); }
}

// ═══════════════════════════════════════════
//  导师信息
// ═══════════════════════════════════════════

void EchoMainWindow::renderMentorMessage()
{
    float a = m_mentorMessageAlpha;
    glClearColor(0.02f * a, 0.02f * a, 0.04f * a, 1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    for (int i = 0;i < 4;++i) { float r = 0.2f + i * 0.18f + 0.03f * sinf(m_gameTime + i);drawCircleGL(0, 0.15f, r, QColor(0, 150, 255, int(a * 40 - i * 8)), 64); }
    drawTextGL(-0.5f, 0.5f, "导师的语音日志 #" + QString::number(m_mentorMessageIndex + 1), QColor(0, 200, 255, int(a * 220)), 0.8f);
    drawTextGL(-0.6f, 0.2f, "\"" + m_currentMentorMessage + "\"", QColor(200, 220, 255, int(a * 200)), 0.55f);
    if (m_showMentorInput) {
        float b = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);drawTextGL(-0.4f, -0.2f, "请输入你的回复（按 ENTER 发送）:", QColor(0, 200, 255, int(a * b * 255)), 0.5f);
        QString dr = m_playerReply; if (int(m_gameTime * 2) % 2 == 0) dr += "|"; drawTextGL(-0.4f, -0.35f, dr, QColor(0, 255, 200, int(a * 220)), 0.6f);
    } else drawTextGL(-0.3f, -0.3f, "按 ENTER 继续", QColor(150, 180, 200, int(a * 150)), 0.5f);
    for (int i = 0;i < 300;++i) {
        float t = i / 300.0f, x = -0.95f + t * 1.9f, y = -0.7f + 0.02f * sinf(t * 50.0f + m_gameTime * 3.0f);
        if (i > 0) { float t2 = (i - 1) / 300.0f, x2 = -0.95f + t2 * 1.9f, y2 = -0.7f + 0.02f * sinf(t2 * 50.0f + m_gameTime * 3.0f);drawLineGL(x2, y2, x, y, QColor(255, 40, 40, int(a * 20)), 0.3f); }
    }
}

// ═══════════════════════════════════════════
//  玩家选择界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderPlayerChoice()
{
    glClearColor(0.03f, 0.03f, 0.06f, 1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    // prompt 放在顶部，选项整体下移，避免重叠
    // prompt 多行展开从 y=0.8 向下，选项从下方固定区域开始
    drawTextGL(-0.8f, 0.8f, m_choicePrompt, QColor(0, 200, 255, 220), 0.5f);
    // 计算 prompt 占用行数，动态下移选项起始位置
    int promptLines = m_choicePrompt.count('\n') + 1;
    // 每行约占 0.06 GL 单位（scale 0.5 下 14px 行高映射）
    float optionStart = 0.8f - promptLines * 0.05f - 0.15f;
    int n = m_choiceOptions.size();
    // 选项从下往上排布，避免与 prompt 重叠且不溢出底部
    float itemH = 0.13f, gap = 0.05f;
    float blockTop = optionStart;
    float blockBottom = blockTop - n * (itemH + gap);
    if (blockBottom < -0.8f) {  // 溢出时整体上移
        blockTop += (-0.8f - blockBottom);
        blockBottom = -0.8f;
    }
    for (int i = 0;i < n;++i) {
        float y = blockTop - i * (itemH + gap);
        QColor bg = m_choiceSelectedIndex == i ? QColor(0, 180, 255, 60) : QColor(20, 30, 50, 40);
        drawRectGL(-0.6f, y - itemH, 1.2f, itemH, bg, true);
        QColor bc = m_choiceSelectedIndex == i ? QColor(0, 200, 255, 200) : QColor(100, 140, 180, 120);
        drawRectGL(-0.6f, y - itemH, 1.2f, itemH, bc, false);
        // 选项文字 y 对齐到框内中部
        drawTextGL(-0.55f, y - itemH / 2.0f - 0.015f, QString("%1. %2").arg(i + 1).arg(m_choiceOptions[i]), QColor(200, 220, 255, 200), 0.45f);
    }
    drawTextGL(-0.3f, -0.85f, "↑↓ 选择 | ENTER 确认 | ESC 返回", QColor(150, 180, 200, 180), 0.4f);
}

void EchoMainWindow::showChoice(const QString& prompt, const QStringList& options)
{
    m_choicePrompt = prompt; m_choiceOptions = options; m_choiceSelectedIndex = 0;
    m_prevState = m_state; m_state = GameState::PlayerChoice;
}

// ═══════════════════════════════════════════
//  结局画面
// ═══════════════════════════════════════════

void EchoMainWindow::renderEndingScreen()
{
    EndingType et = m_engine->calculateEnding();
    m_engine->unlockEnding(et);  // 结局一旦展示即标记解锁（幂等，供多周目收集）
    EndingInfo ei = m_engine->getEndingInfo(et);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0;i < 5;++i) { float r = 0.3f + i * 0.2f + 0.03f * sinf(m_gameTime + i), a = 0.1f - i * 0.02f;drawCircleGL(0, 0.1f, r, QColor(0, 150, 255, int(a * 255)), 64); }
    drawTextGL(-0.3f, 0.6f, "结局：" + ei.title, QColor(0, 200, 255, 220), 1.2f);
    QString desc = ei.description;
    drawTextGL(-0.7f, 0.2f, desc, QColor(180, 200, 220, 180), 0.45f);
    // 多周目收集进度（接入 unlockedEndingCount）
    int uec = m_engine->unlockedEndingCount();
    int total = m_engine->totalEndingCount();
    drawTextGL(-0.7f, -0.25f, QString("结局收集：%1/%2").arg(uec).arg(total), QColor(150, 180, 200, 160), 0.4f);
    // 隐藏关解锁提示（接入 isResonancePeaked / isFirstSoundUnlocked）
    if (m_engine->isResonancePeaked() && !m_engine->isFirstSoundUnlocked()) {
        float a = 0.5f + 0.3f * sinf(m_gameTime * 4.0f);
        drawTextGL(-0.7f, -0.35f, "✦ 共鸣度已满——隐藏关「第一声」已可解锁", QColor(255, 210, 100, int(a * 255)), 0.45f);
    } else if (m_engine->isFirstSoundUnlocked()) {
        drawTextGL(-0.7f, -0.35f, "✦ 隐藏关「第一声」已解锁", QColor(255, 210, 100, 180), 0.45f);
    }
    // 被遗忘者名单（接入 forgottenListText，右侧栏展示）
    if (m_engine->unlockedEchoFragmentCount() > 0) {
        QString fl = m_engine->forgottenListText();
        drawTextGL(0.2f, 0.5f, fl, QColor(200, 190, 160, 170), 0.32f);
    }
    float b = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);
    drawTextGL(-0.3f, -0.55f, "按 ENTER 聆听余波录音", QColor(0, 200, 255, int(b * 200)), 0.6f);
}

// ═══════════════════════════════════════════
//  结局余波录音
// ═══════════════════════════════════════════

void EchoMainWindow::renderEpilogueScreen()
{
    float a = m_epilogueAlpha;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);glClear(GL_COLOR_BUFFER_BIT);
    // 极暗的呼吸光晕
    for (int i = 0;i < 3;++i) { float r = 0.2f + i * 0.15f + 0.02f * sinf(m_gameTime * 0.5f + i), al = 0.03f * a - i * 0.008f;drawCircleGL(0, -0.1f, r, QColor(0, 100, 180, int(al * 255)), 64); }
    drawTextGL(-0.5f, 0.5f, "—— 余波录音 ——", QColor(120, 160, 200, int(a * 180)), 0.8f);
    drawTextGL(-0.7f, 0.2f, m_epilogueText, QColor(160, 180, 200, int(a * 180)), 0.42f);
    if (m_epilogueFinished) { float b = 0.6f + 0.4f * sinf(m_gameTime * 3.0f);drawTextGL(-0.3f, -0.7f, "按 ENTER 返回标题", QColor(0, 200, 255, int(b * 200)), 0.6f); } else { drawTextGL(-0.2f, -0.7f, "……", QColor(80, 100, 130, int(a * 120)), 0.6f); }
}

// ═══════════════════════════════════════════
//  回声碎片档案
// ═══════════════════════════════════════════

void EchoMainWindow::renderEchoArchive()
{
    glClearColor(0.04f, 0.04f, 0.07f, 1.0f);glClear(GL_COLOR_BUFFER_BIT);
    drawTextGL(-0.9f, 0.85f, "回声碎片档案（被遗忘者的回声）", QColor(0, 200, 255, 220), 0.9f);
    drawTextGL(0.2f, 0.85f, QString("已解锁 %1/6").arg(m_engine->unlockedEchoFragmentCount()), QColor(150, 200, 220, 180), 0.5f);
    auto& frags = m_engine->echoFragments();
    float py = 0.68f, ih = 0.32f, gap = 0.06f;
    float so = m_echoArchiveScroll;
    for (int i = 0;i < static_cast<int>(frags.size());++i) {
        float y = py - (ih + gap) * i + so;
        if (y > 1.2f || y < -1.2f) continue;
        bool unlocked = m_engine->isEchoFragmentUnlocked(i);
        QColor bg = unlocked ? QColor(0, 100, 150, 40) : QColor(30, 30, 40, 40);
        drawRectGL(-0.9f, y - ih + 0.04f, 1.8f, ih - 0.04f, bg, true);
        QColor bc = unlocked ? QColor(0, 200, 255, 120) : QColor(80, 90, 110, 80);
        drawRectGL(-0.9f, y - ih + 0.04f, 1.8f, ih - 0.04f, bc, false);
        QString title = (unlocked ? frags[i].title : QString("???"));
        drawTextGL(-0.85f, y, title, unlocked ? QColor(0, 220, 255, 220) : QColor(100, 110, 130, 150), 0.55f);
        drawTextGL(-0.85f, y - 0.07f, (unlocked ? frags[i].location : "[未解锁]"), unlocked ? QColor(150, 180, 200, 160) : QColor(80, 90, 110, 120), 0.35f);
        if (unlocked) {
            // 内容文字自动换行（由 drawTextGL 处理 \n，按窗口宽度自适应折行）
            drawTextGL(-0.85f, y - 0.13f, frags[i].content, QColor(180, 200, 220, 160), 0.32f);
        }
    }
    drawTextGL(-0.9f, -0.9f, "↑↓/滚轮 浏览 | ESC 返回", QColor(100, 120, 140, 120), 0.4f);
    // 已揭示角色档案（接入 characters() + revealCharacter）
    drawTextGL(0.3f, 0.85f, "已揭示角色", QColor(0, 200, 255, 220), 0.6f);
    const auto& chars = m_engine->characters();
    int cy = 0;
    for (size_t i = 0;i < chars.size();++i) {
        if (!chars[i].isRevealed) continue;
        float y = 0.72f - cy * 0.22f;
        if (y < -0.6f) break;
        QString nm = chars[i].name;
        // 角色名占位符替换（{name} → 玩家名）
        nm.replace(PLAYER_NAME_PLACEHOLDER, m_engine->playerDisplayName());
        drawTextGL(0.3f, y, nm, QColor(0, 220, 255, 220), 0.5f);
        drawTextGL(0.3f, y - 0.06f, chars[i].title, QColor(150, 180, 200, 160), 0.35f);
        drawTextGL(0.3f, y - 0.12f, chars[i].secret, QColor(160, 180, 200, 140), 0.28f);
        cy++;
    }
}

// ═══════════════════════════════════════════
//  共鸣度金色波纹（耳蜗共鸣）
// ═══════════════════════════════════════════

void EchoMainWindow::renderResonanceAura()
{
    float level = m_resonanceDisplay;
    // 屏幕边缘金色波纹
    int ringCount = int(level * 6.0f);
    for (int i = 0;i < ringCount;++i) {
        float rr = 0.3f + i * 0.12f + 0.03f * sinf(m_resonanceAuraPhase + i * 1.3f);
        float al = level * 0.25f - i * 0.03f;if (al < 0.0f)al = 0.0f;
        drawCircleGL(0, 0, rr, QColor(255, 200, 80, int(al * 255)), 96);
    }
    // 左上角共鸣度指示（基于可见范围，避免窄屏裁切）
    float asp = float(m_winWidth) / float(m_winHeight);
    float leftEdge = asp > 1.0f ? -asp : -1.0f;
    drawTextGL(leftEdge + 0.05f, 0.95f, QString("共鸣度 %1%").arg(int(level * 100)), QColor(255, 210, 100, 180), 0.4f);
}

// ═══════════════════════════════════════════
//  巧妙机制 1：陈远山的"静音惩罚"（右耳失聪）
// ═══════════════════════════════════════════

void EchoMainWindow::renderSilencePunishment()
{
    // 右侧频谱被"静音"——右耳失聪的视觉隐喻
    float aspect = float(m_winWidth) / float(m_winHeight);
    // 覆盖右侧半屏的暗色遮罩，带微弱噪点，暗示右声道消失
    float x0 = 0.0f;  // 从屏幕中心开始，覆盖右半
    float w = (aspect > 1.0f ? aspect : 1.0f);
    float h = (aspect > 1.0f ? 1.0f : 1.0f / aspect);
    // 极暗遮罩
    drawRectGL(x0, -h, w, 2.0f * h, QColor(2, 2, 4, 210), true);
    // 稀疏噪点（残存的声音碎屑）
    for (int i = 0;i < 40;++i) {
        float nx = x0 + (rand() % 100) / 100.0f * w;
        float ny = -h + (rand() % 200) / 100.0f * h;
        float s = (rand() % 4) / 400.0f;
        drawRectGL(nx, ny, s, s, QColor(0, 120, 200, 30), true);
    }
    // 提示文字（在右侧）
    drawTextGL(x0 + 0.05f, 0.2f, "（右声道）", QColor(80, 100, 130, 120), 0.5f);
    drawTextGL(x0 + 0.05f, 0.1f, "……你正逐渐失去", QColor(120, 140, 170, 140), 0.4f);
    drawTextGL(x0 + 0.05f, 0.02f, "一半的声音", QColor(160, 80, 80, 160), 0.45f);
}

// ═══════════════════════════════════════════
//  巧妙机制 3：碎片回声（衰减波纹）
// ═══════════════════════════════════════════

void EchoMainWindow::renderFragmentEchoes()
{
    const auto& echoes = m_engine->echoes();
    for (const auto& e : echoes) {
        float lifeRatio = e.remainingTime / e.maxTime;
        float rr = (1.0f - lifeRatio) * 0.6f + 0.1f;  // 波纹向外扩散
        float al = lifeRatio * e.intensity * 0.5f;
        if (al < 0.0f) al = 0.0f;
        // 每个回声以随机位置为中心扩散（视觉上像声音在空间回荡）
        float cx = ((e.sourceFragmentId * 37) % 200 - 100) / 100.0f;
        float cy = ((e.sourceFragmentId * 53) % 160 - 80) / 100.0f;
        drawCircleGL(cx, cy, rr, QColor(0, 200, 255, int(al * 255)), 48);
    }
}

// ═══════════════════════════════════════════
//  巧妙机制 4：老刘的静默摩斯
// ═══════════════════════════════════════════

void EchoMainWindow::renderSilenceMorse()
{
    QString text = m_engine->getSilenceMorseReveal();
    if (text.isEmpty()) return;
    // 屏幕下方半透明浮动文字，淡入淡出
    float alpha = 0.5f + 0.2f * sinf(m_gameTime * 1.5f);
    drawRectGL(-0.7f, -0.75f, 1.4f, 0.3f, QColor(5, 8, 15, 180), true);
    drawTextGL(-0.65f, -0.55f, text, QColor(180, 200, 220, int(alpha * 200)), 0.4f);
}

// ═══════════════════════════════════════════
//  视觉效果
// ═══════════════════════════════════════════

void EchoMainWindow::renderGlitchEffects()
{
    float i = m_glitchIntensity;
    for (int j = 0;j < 5 * i;++j) { float y = (rand() % 200 - 100) / 100.0f, o = (rand() % 20 - 10) / 100.0f * i, a = i * (rand() % 100) / 100.0f;drawLineGL(-1.0f + o, y, 1.0f + o, y, QColor(255, 0, 0, int(a * 80)), 1.0f); }
    for (auto& p : m_glitchParticles) { float a = p.life / p.maxLife;drawRectGL(p.x, p.y, 0.005f, 0.005f, QColor(p.color.red(), p.color.green(), p.color.blue(), int(a * p.color.alpha())), true); }
    if (rand() % 100 < int(i * 30)) { float fa = i * 0.08f;glClearColor(0.15f * fa, 0.02f * fa, 0.02f * fa, 1.0f); }
}

void EchoMainWindow::renderParanoiaMeter()
{
    // 右侧指示器，基于可见宽度自适应，避免窄屏溢出
    float asp = float(m_winWidth) / float(m_winHeight);
    float rightEdge = asp > 1.0f ? asp : 1.0f;
    float mx = rightEdge - 0.08f, my = 0.8f, mw = 0.04f, mh = 0.3f;
    drawRectGL(mx, my - mh, mw, mh, QColor(10, 10, 20, 120), true); drawRectGL(mx, my - mh, mw, mh, QColor(60, 80, 120, 40), false);
    float fh = mh * m_paranoiaDisplay;
    QColor fc = m_paranoiaDisplay < 0.3f ? QColor(0, 180, 255, 150) : (m_paranoiaDisplay < 0.6f ? QColor(255, 200, 0, 150) : QColor(255, 40, 40, 180));
    drawRectGL(mx, my - fh, mw, fh, fc, true);
    drawTextGL(mx - 0.05f, my + 0.04f, "听觉偏执", QColor(150, 180, 200, 150), 0.25f);
    drawTextGL(mx - 0.01f, my - mh - 0.02f, QString("%1%").arg(int(m_paranoiaDisplay * 100)), fc, 0.25f);
}

void EchoMainWindow::renderAnxietyMeter()
{
    float asp = float(m_winWidth) / float(m_winHeight);
    float rightEdge = asp > 1.0f ? asp : 1.0f;
    float mx = rightEdge - 0.08f, my = 0.35f, mw = 0.04f, mh = 0.3f;
    drawRectGL(mx, my - mh, mw, mh, QColor(10, 10, 20, 120), true); drawRectGL(mx, my - mh, mw, mh, QColor(120, 60, 60, 40), false);
    float fh = mh * m_anxietyDisplay;
    QColor fc = m_anxietyDisplay < 0.4f ? QColor(0, 200, 150, 120) : (m_anxietyDisplay < 0.7f ? QColor(255, 180, 0, 150) : QColor(255, 40, 40, 180));
    drawRectGL(mx, my - fh, mw, fh, fc, true);
    drawTextGL(mx - 0.05f, my + 0.04f, "焦虑值", QColor(200, 150, 150, 150), 0.25f);
}

void EchoMainWindow::renderSpectrumVisualization()
{
    float by = -0.95f, mh = 0.15f;
    for (size_t i = 0;i < m_spectrumLines.size();++i) { auto& l = m_spectrumLines[i];float x = -0.95f + i * 1.9f / m_spectrumLines.size(), h = l.amplitude * mh * (0.5f + 0.5f * m_paranoiaDisplay);QColor c = l.color;c.setAlpha(int(l.amplitude * 80));drawRectGL(x, by, 1.9f / m_spectrumLines.size() - 0.001f, h, c, true); }
    if (m_paranoiaDisplay > 0.5f) for (size_t i = 0;i < m_spectrumLines.size() / 8;++i) { auto& l = m_spectrumLines[i];float x = -0.95f + i * 1.9f / m_spectrumLines.size(), h = l.amplitude * mh * m_paranoiaDisplay;int a = int(m_paranoiaDisplay * l.amplitude * 120);drawRectGL(x, by, 1.9f / m_spectrumLines.size() - 0.001f, h, QColor(255, 40, 40, a), true); }
}

void EchoMainWindow::renderHallucinations()
{
    for (auto& h : m_hallucinations) drawTextGL(h.x, h.y, h.text, QColor(255, 200, 100, int(h.alpha * 150)), 0.5f);
    // 高偏执时周期性浮现幻觉（接入 getHallucinationText，不再只依赖信号）
    QString ht = m_engine->getHallucinationText();
    if (!ht.isEmpty() && (rand() % 100) < 8) {
        float x = (rand() % 160 - 80) / 100.0f, y = (rand() % 120 - 60) / 100.0f;
        float a = 0.3f + 0.2f * sinf(m_gameTime * 3.0f);
        drawTextGL(x, y, ht, QColor(255, 180, 80, int(a * 255)), 0.45f);
    }
}

void EchoMainWindow::renderNoiseDialogues()
{
    for (auto& n : m_noiseTexts) drawTextGL(n.x, n.y, n.text, QColor(100, 255, 200, int(n.alpha * 120)), 0.45f);
    // 底噪意识对话（接入 canCommuneWithNoise + noiseDialogue）
    if (m_engine->canCommuneWithNoise() && (rand() % 100) < 10) {
        QString nd = m_engine->noiseDialogue();
        if (!nd.isEmpty()) {
            float x = -0.85f + (rand() % 120) / 100.0f, y = 0.3f - (rand() % 80) / 100.0f;
            float a = 0.35f + 0.2f * sinf(m_gameTime * 2.5f);
            drawTextGL(x, y, nd, QColor(100, 255, 200, int(a * 180)), 0.45f);
        }
    }
}

void EchoMainWindow::renderSilenceOverlay()
{
    float a = m_silenceAlpha;
    drawRectGL(-1.5f, -1.0f, 3.0f, 2.0f, QColor(0, 0, 0, int(a * 200)), true);
    if (a > 0.5f) { float b = 0.5f + 0.3f * sinf(m_gameTime * 2.0f);drawTextGL(-0.3f, 0.0f, "静默中……", QColor(255, 255, 255, int(b * 150)), 0.8f); }
}

// ═══════════════════════════════════════════
//  耳鸣指引（林薇的潜意识引导）
// ═══════════════════════════════════════════

void EchoMainWindow::renderTinnitusGuide()
{
    float asp = float(m_winWidth) / float(m_winHeight);
    float ghw = (asp > 1.0f ? asp : 1.0f), ghh = (asp > 1.0f ? 1.0f : 1.0f / asp);

    // 1) 耳鸣波纹：屏幕中央向外扩散的极淡同心圆
    float ringPhase = m_tinnitusPhase * 2.0f;
    for (int i = 0;i < 3;++i) {
        float rr = 0.15f + fmodf(ringPhase * 0.5f + i * 0.2f, 0.8f);
        float al = 0.10f * (1.0f - rr / 0.95f); if (al < 0.0f)al = 0.0f;
        drawCircleGL(0.0f, 0.0f, rr, QColor(200, 200, 255, int(al * 255)), 64);
    }

    // 2) 确定指引目标位置（GL 坐标）
    float tx = 0.0f, ty = 0.0f;
    bool hasTarget = false;
    if (m_state == GameState::FragmentSelect || m_state == GameState::Tutorial) {
        // 指向第一个未放置的碎片
        const auto& frags = m_engine->fragments();
        for (const auto& f : frags) {
            if (!f.isPlaced) { tx = -0.85f; ty = 0.55f; hasTarget = true; break; }
        }
    } else if (m_state == GameState::TimelinePuzzle) {
        // 指向第一个空槽位
        const auto& tl = m_engine->timeline();
        for (size_t i = 0;i < tl.size();++i) {
            if (tl[i].fragmentId < 0) {
                int n = m_engine->timelineSlotCount();
                float slotW = std::min(0.1f, (TIMELINE_W - (n - 1) * SLOT_GAP) / n);
                float totalW = n * slotW + (n - 1) * SLOT_GAP;
                float sx0 = TIMELINE_X + (TIMELINE_W - totalW) / 2.0f;
                tx = sx0 + i * (slotW + SLOT_GAP) + slotW / 2.0f;
                ty = TIMELINE_Y + 0.1f;
                hasTarget = true; break;
            }
        }
    }
    if (!hasTarget) return;

    // 3) 白色箭头指向目标（屏幕边缘闪烁 + 指向目标）
    float arrowAlpha = 0.35f + 0.25f * sinf(m_gameTime * 4.0f);
    // 从屏幕边缘（根据目标方位）指向目标
    float dirX = tx; float dirY = ty;
    float len = sqrtf(dirX * dirX + dirY * dirY);
    if (len < 0.01f) len = 0.01f;
    float ux = dirX / len, uy = dirY / len;
    // 箭头起点：屏幕边缘（目标方向的反方向边缘）
    float ex = ghw * ux, ey = ghh * uy;
    // 绘制箭头线
    drawLineGL(ex, ey, tx, ty, QColor(255, 255, 255, int(arrowAlpha * 255)), 2.0f);
    // 箭头头部（两条短斜线）
    float headLen = 0.08f;
    float ang = atan2f(ty - ey, tx - ex);
    drawLineGL(tx, ty, tx - headLen * cosf(ang - 0.5f), ty - headLen * sinf(ang - 0.5f), QColor(255, 255, 255, int(arrowAlpha * 255)), 2.0f);
    drawLineGL(tx, ty, tx - headLen * cosf(ang + 0.5f), ty - headLen * sinf(ang + 0.5f), QColor(255, 255, 255, int(arrowAlpha * 255)), 2.0f);

    // 4) 耳鸣提示文字（后期章节揭示是林薇）
    GameChapter ch = m_engine->currentChapter();
    QString hint = (ch >= GameChapter::Chapter3)
        ? "（一阵耳鸣……是她在你脑内指引）"
        : "（耳鸣声……）";
    float ha = 0.4f + 0.2f * sinf(m_gameTime * 3.0f);
    drawTextGL(-0.9f, 0.92f, hint, QColor(220, 220, 255, int(ha * 180)), 0.38f);
}

void EchoMainWindow::renderReverseIndicator()
{
    if (m_engine->isReverseUnlocked()) {
        float b = 0.5f + 0.3f * sinf(m_gameTime * 2.0f);
        drawTextGL(-0.9f, -0.95f, "◄ 逆向播放已解锁", QColor(255, 100, 200, int(b * 180)), 0.35f);
    }
}

void EchoMainWindow::renderMorseIndicator()
{
    if (m_engine->isMorseActive()) {
        QString m = m_engine->getMorseMessage();
        if (!m.isEmpty()) { float b = 0.5f + 0.4f * sinf(m_gameTime * 5.0f);drawTextGL(0.5f, -0.95f, "[摩斯] " + m, QColor(255, 200, 100, int(b * 150)), 0.3f); }
    }
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

QPointF EchoMainWindow::screenToGL(const QPoint& sp) const
{
    float asp = float(m_winWidth) / float(m_winHeight), ghw, ghh;
    if (asp > 1.0f) { ghw = asp;ghh = 1.0f; } else { ghw = 1.0f;ghh = 1.0f / asp; }
    return QPointF((sp.x() / float(m_winWidth)) * 2.0f * ghw - ghw, ghh - (sp.y() / float(m_winHeight)) * 2.0f * ghh);
}

int EchoMainWindow::hitTestFragment(const QPointF& gp) const
{
    auto& frags = m_engine->fragments();
    if (m_state == GameState::FragmentSelect || m_state == GameState::Tutorial) {
        // 与 renderFragmentSelect 保持一致的动态 py 计算
        QString desc = m_engine->chapterDescription();
        int descLines = desc.count('\n') + 1;
        float py = 0.72f - descLines * 0.04f - 0.15f;
        float px = -0.9f, pw = 0.85f, ih = 0.11f, gap = 0.02f, so = m_fragmentScrollY;
        for (size_t i = 0;i < frags.size();++i) { auto& f = frags[i];if (f.isPlaced)continue;float y = py + (ih + gap) * float(i) + so;if (gp.x() >= px && gp.x() <= px + pw && gp.y() >= y - ih && gp.y() <= y)return int(i); }
    } else {
        float pnx = -0.95f, pny = 0.65f, iw = 0.25f, ih = 0.06f, gap = 0.01f;int col = 0, row = 0, mc = 3;
        for (size_t i = 0;i < frags.size();++i) { auto& f = frags[i];if (f.isPlaced) { col++;if (col >= mc) { col = 0;row++; }continue; }float x = pnx + col * (iw + gap), y = pny - row * (ih + gap * 3);if (gp.x() >= x && gp.x() <= x + iw && gp.y() >= y - ih && gp.y() <= y)return int(i);col++;if (col >= mc) { col = 0;row++; } }
    }
    return -1;
}

int EchoMainWindow::hitTestTimelineSlot(const QPointF& gp) const
{
    int sc = m_engine->timelineSlotCount();
    float sw = std::min(0.1f, (TIMELINE_W - (sc - 1) * SLOT_GAP) / sc), tsw = sc * sw + (sc - 1) * SLOT_GAP, ssx = TIMELINE_X + (TIMELINE_W - tsw) / 2.0f;
    for (int i = 0;i < sc;++i) { float sx = ssx + i * (sw + SLOT_GAP), sy = TIMELINE_Y + 0.02f, sh = TIMELINE_H - 0.04f;if (gp.x() >= sx && gp.x() <= sx + sw && gp.y() >= sy && gp.y() <= sy + sh)return i; }
    return -1;
}

// ═══════════════════════════════════════════
//  输入处理
// ═══════════════════════════════════════════

void EchoMainWindow::keyPressEvent(QKeyEvent* e)
{
    m_idleTime = 0.0f;  // 玩家操作打断静默
    switch (m_state) {
    case GameState::Title:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) { m_playerName.clear();m_state = GameState::NameInput; } else if (e->key() == Qt::Key_D) {
            // 进入声纹推演盘（统一入口，从标题画面切换）
            if (!m_deductionBoard) {
                m_deductionBoard = new DeductionBoardWidget;
                m_deductionBoard->setAttribute(Qt::WA_DeleteOnClose);
                m_deductionBoard->setWindowTitle("声纹推演盘 - 余音回响");
            }
            m_deductionBoard->show();
            m_deductionBoard->raise();
            m_deductionBoard->activateWindow();
        }
        break;
    case GameState::NameInput:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            m_engine->setPlayerName(m_playerName.trimmed());
            m_engine->generateFragments(GameChapter::Prologue);
            showChapterIntro();
        } else if (e->key() == Qt::Key_Escape) { m_playerName.clear();m_state = GameState::Title; } else if (e->key() == Qt::Key_Backspace) { if (!m_playerName.isEmpty())m_playerName.chop(1); } else if (!e->text().isEmpty() && e->text().at(0).isPrint()) { if (m_playerName.length() < 12)m_playerName += e->text(); }
        break;
    case GameState::ChapterIntro:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            if (m_engine->currentChapter() == GameChapter::Prologue) m_state = GameState::Tutorial;
            else m_state = GameState::FragmentSelect;
            m_chapterIntroShown = true;
        }
        break;
    case GameState::Tutorial:
        if (e->key() == Qt::Key_Escape) m_state = GameState::Title;
        else if (e->key() == Qt::Key_Tab) { m_state = GameState::TimelinePuzzle;m_selectedFragmentIndex = -1; }
        break;
    case GameState::FragmentSelect:
        if (e->key() == Qt::Key_Escape) { m_state = GameState::Title;m_selectedFragmentIndex = -1; } else if (e->key() == Qt::Key_Tab) { m_state = GameState::TimelinePuzzle; } else if (e->key() == Qt::Key_E) { m_echoArchiveScroll = 0.0f;m_state = GameState::EchoArchive; } else if (e->key() == Qt::Key_R && m_selectedFragmentIndex >= 0) {
            auto& frags = m_engine->fragments();
            if (m_selectedFragmentIndex<int(frags.size())) {
                m_engine->reverseFragment(frags[m_selectedFragmentIndex].id);
            }
        }
        break;
    case GameState::TimelinePuzzle:
        if (e->key() == Qt::Key_Escape) { m_state = GameState::FragmentSelect;m_selectedFragmentIndex = -1; } else if (e->key() == Qt::Key_R) { m_engine->resetCurrentChapter();m_selectedFragmentIndex = -1;m_glitchIntensity = 0.0f; } else if (e->key() == Qt::Key_Tab && m_selectedFragmentIndex >= 0) {
            auto& frags = m_engine->fragments();
            if (m_selectedFragmentIndex<int(frags.size())) {
                m_engine->reverseFragment(frags[m_selectedFragmentIndex].id);
            }
        }
        break;
    case GameState::SceneComplete:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            GameChapter cur = m_engine->currentChapter();
            if (cur == GameChapter::Prologue) {
                auto cc = m_engine->buildChoice(ChoicePoint::PrologueSigh);
                showChoice(cc.prompt, cc.options);
            } else if (cur == GameChapter::Chapter3) {
                // 第三章：先触发何悦被囚禁的支线抉择，再进入自我声纹选择
                if (m_engine->heYueState() == HeYueState::Unknown) {
                    auto cc = m_engine->buildChoice(ChoicePoint::HeYueHostage);
                    showChoice(cc.prompt, cc.options);
                } else {
                    auto cc = m_engine->buildChoice(ChoicePoint::Identity);
                    showChoice(cc.prompt, cc.options);
                }
            } else if (cur == GameChapter::Chapter4) {
                auto cc = m_engine->buildChoice(ChoicePoint::LifeTransfer);
                showChoice(cc.prompt, cc.options);
            } else {
                GameChapter next = static_cast<GameChapter>(static_cast<int>(cur) + 1);
                if (next <= GameChapter::Finale) {
                    m_currentMentorMessage = m_engine->getMentorMessage(static_cast<int>(cur));
                    m_mentorMessageIndex = static_cast<int>(cur);
                    m_mentorMessageAlpha = 0.0f;
                    m_showMentorInput = (cur == GameChapter::Chapter3);
                    m_state = GameState::MentorMessage;
                } else {
                    m_state = GameState::EndingScreen;
                }
            }
        } else if (e->key() == Qt::Key_R) { m_engine->resetCurrentChapter();m_selectedFragmentIndex = -1;m_glitchIntensity = 0.0f;m_state = GameState::FragmentSelect; }
        break;
    case GameState::MentorMessage:
        if (m_showMentorInput) {
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                m_showMentorInput = false;
                GameChapter next = static_cast<GameChapter>(static_cast<int>(m_engine->currentChapter()) + 1);
                if (next <= GameChapter::Finale) { m_engine->generateFragments(next);showChapterIntro(); } else { m_state = GameState::EndingScreen; }
                m_selectedFragmentIndex = -1;
            } else if (e->key() == Qt::Key_Backspace) { if (!m_playerReply.isEmpty())m_playerReply.chop(1); } else if (!e->text().isEmpty() && e->text().at(0).isPrint())m_playerReply += e->text();
        } else {
            if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                GameChapter next = static_cast<GameChapter>(static_cast<int>(m_engine->currentChapter()) + 1);
                if (next <= GameChapter::Finale) { m_engine->generateFragments(next);showChapterIntro(); } else { m_state = GameState::EndingScreen; }
                m_selectedFragmentIndex = -1;
            }
        }break;
    case GameState::PlayerChoice:
        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_W) { if (m_choiceSelectedIndex > 0)m_choiceSelectedIndex--; } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_S) { if (m_choiceSelectedIndex < m_choiceOptions.size() - 1)m_choiceSelectedIndex++; } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            PlayerChoice pc = PlayerChoice::None;
            GameChapter cur = m_engine->currentChapter();
            if (cur == GameChapter::Prologue) {
                pc = (m_choiceSelectedIndex == 0) ? PlayerChoice::IgnoreSigh : PlayerChoice::RecordSigh;
                m_engine->recordChoice(pc);
                // 接入官方锚点机制：忽略叹息=相信官方锚点，记录叹息=放弃官方锚点靠自己判断
                m_engine->setOfficialAnchor(m_choiceSelectedIndex == 0);
                m_engine->generateFragments(GameChapter::Chapter1);
                showChapterIntro();
            } else if (cur == GameChapter::Chapter3) {
                if (m_engine->heYueState() == HeYueState::Unknown) {
                    // 何悦支线抉择
                    if (m_choiceSelectedIndex == 0) {
                        m_engine->setHeYueState(HeYueState::Rescued);
                        m_engine->addResonance(0.15f);
                    } else {
                        m_engine->setHeYueState(HeYueState::Brainwashed);
                    }
                    // 无论救没救，都继续自我声纹选择（文案由 buildChoice 根据状态动态拼接）
                    auto cc = m_engine->buildChoice(ChoicePoint::Identity);
                    showChoice(cc.prompt, cc.options);
                    return;
                }
                if (m_choiceSelectedIndex == 0) pc = PlayerChoice::AcceptFusion;
                else if (m_choiceSelectedIndex == 1) pc = PlayerChoice::RejectFusion;
                else pc = PlayerChoice::SeparateVoices;
                m_engine->recordChoice(pc);
                m_engine->generateFragments(GameChapter::Chapter4);
                showChapterIntro();
            } else if (cur == GameChapter::Chapter4) {
                pc = (m_choiceSelectedIndex == 0) ? PlayerChoice::TransferBack : PlayerChoice::DeleteLifeSupport;
                m_engine->recordChoice(pc);
                m_state = GameState::EndingScreen;
            }
        } else if (e->key() == Qt::Key_Escape) { m_state = m_prevState; }
        break;
    case GameState::EndingScreen:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            // 进入结局余波录音
            EndingType et = m_engine->calculateEnding();
            m_epilogueText = m_engine->getEpilogue(et);
            if (!m_epilogueText.isEmpty()) {
                m_epilogueAlpha = 0.0f;m_epilogueTimer = 0.0f;m_epilogueFinished = false;
                m_state = GameState::EpilogueScreen;
            } else {
                m_state = GameState::GameOver;
            }
        }
        break;
    case GameState::EpilogueScreen:
        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && m_epilogueFinished) m_state = GameState::GameOver;
        break;
    case GameState::EchoArchive:
        if (e->key() == Qt::Key_Escape) m_state = GameState::FragmentSelect;
        else if (e->key() == Qt::Key_Up || e->key() == Qt::Key_W) { m_echoArchiveScroll += 0.2f; } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_S) { m_echoArchiveScroll -= 0.2f; }
        break;
    case GameState::GameOver:
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) { m_engine->generateFragments(GameChapter::Prologue);showChapterIntro();m_selectedFragmentIndex = -1;m_mentorMessageIndex = 0;m_playerReply.clear(); }
        break;
    }
    update();
}

void EchoMainWindow::mousePressEvent(QMouseEvent* e)
{
    m_idleTime = 0.0f;  // 玩家操作打断静默
    QPointF gp = screenToGL(e->pos());
    if (m_state == GameState::FragmentSelect || m_state == GameState::TimelinePuzzle || m_state == GameState::Tutorial) {
        if (e->button() == Qt::LeftButton) { int fi = hitTestFragment(gp);if (fi >= 0) { m_selectedFragmentIndex = fi;m_isDragging = true;m_dragStartPos = e->pos();m_dragCurrentPos = e->pos();if (m_state == GameState::FragmentSelect || m_state == GameState::Tutorial)m_state = GameState::TimelinePuzzle; } } else if (e->button() == Qt::RightButton) { m_selectedFragmentIndex = -1;m_isDragging = false; }
    }
}

void EchoMainWindow::mouseMoveEvent(QMouseEvent* e) { if (m_isDragging) m_dragCurrentPos = e->pos(); }

void EchoMainWindow::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_isDragging && e->button() == Qt::LeftButton) {
        m_isDragging = false;QPointF gp = screenToGL(e->pos());
        if (m_state == GameState::TimelinePuzzle && m_selectedFragmentIndex >= 0) {
            int si = hitTestTimelineSlot(gp);
            if (si >= 0) { auto& frags = m_engine->fragments();if (m_selectedFragmentIndex<int(frags.size())) { m_engine->placeFragment(frags[m_selectedFragmentIndex].id, si); } }m_selectedFragmentIndex = -1;
        }
    }
    if (e->button() == Qt::RightButton && m_state == GameState::TimelinePuzzle) { QPointF gp = screenToGL(e->pos());int si = hitTestTimelineSlot(gp);if (si >= 0)m_engine->removeFragment(si); }
}

// ═══════════════════════════════════════════
//  信号槽
// ═══════════════════════════════════════════

void EchoMainWindow::onFragmentPlaced(int, int)
{
    // 放置碎片 = 解谜推进，重置耳鸣指引计时
    m_stuckTime = 0.0f;
    m_tinnitusActive = false;
}
void EchoMainWindow::onFragmentRemoved(int, int) {}
void EchoMainWindow::onSceneCompleted(int ch, bool d)
{
    m_state = GameState::SceneComplete;
    // chapter index 映射到 mentor message index（0=Prologue→msg0, 1=Ch1→msg1, ...）
    // 最多 9 条消息，做边界保护
    m_mentorMessageIndex = std::min(ch, 8);
    if (d) m_glitchIntensity = 1.0f;
}
void EchoMainWindow::onParanoiaChanged(float) {}
void EchoMainWindow::onAnxietyChanged(float) {}
void EchoMainWindow::onReverseUnlocked() {}
void EchoMainWindow::onHallucinationTriggered(const QString& m)
{
    HallucinationText ht;ht.text = m;ht.x = (rand() % 160 - 80) / 100.0f;ht.y = (rand() % 120 - 60) / 100.0f;ht.alpha = 0.8f;ht.speedX = (rand() % 20 - 10) / 200.0f;ht.speedY = (rand() % 20 - 10) / 200.0f;ht.life = 4.0f + (rand() % 30) / 10.0f;m_hallucinations.push_back(ht);
}
void EchoMainWindow::onNoiseCommunion(const QString& m)
{
    FloatingNoise fn;fn.text = m;fn.x = (rand() % 100 - 50) / 100.0f;fn.y = -0.3f + (rand() % 60) / 100.0f;fn.alpha = 0.7f;fn.life = 3.0f + (rand() % 20) / 10.0f;m_noiseTexts.push_back(fn);
}
