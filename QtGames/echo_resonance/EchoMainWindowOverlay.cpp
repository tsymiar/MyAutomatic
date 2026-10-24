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
//  结局 / 档案 / 机制特效渲染
// ═══════════════════════════════════════════

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
//  声灵显形光晕（离奇魔幻基因：拼图即召唤）
// ═══════════════════════════════════════════

void EchoMainWindow::renderSpiritAura()
{
    // 仅在时间轴拼图状态显示声灵（碎片选择状态通过漂浮文字提示）
    if (m_state != GameState::TimelinePuzzle) return;
    const auto& spirits = m_engine->spirits();
    if (spirits.empty()) return;
    // 顶部中间偏左横向排列，避开右侧偏执表与场景一致性文字
    float x0 = -0.9f, y = 0.72f, gap = 0.18f;
    for (size_t i = 0; i < spirits.size(); ++i) {
        const auto& s = spirits[i];
        float x = x0 + i * gap;
        QColor c = s.color();
        if (s.manifested) {
            float pulse = 0.5f + 0.5f * sinf(m_gameTime * 3.0f + float(i));
            c.setAlpha(int(130 + 125 * pulse));
            drawCircleGL(x, y, 0.03f, c, 24);
            // 过度依赖（反噬）时闪红圈警示
            if (m_engine->isSpiritBacklashing(s.kind)) {
                drawCircleGL(x, y, 0.042f, QColor(255, 60, 60, int(110 + 130 * pulse)), 24);
            }
            drawTextGL(x - 0.05f, y - 0.045f, s.name, c, 0.35f);
        } else {
            drawCircleGL(x, y, 0.025f, QColor(60, 70, 90, 80), 20);
            drawTextGL(x - 0.05f, y - 0.045f, "？？？", QColor(90, 100, 120, 110), 0.35f);
        }
    }
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
