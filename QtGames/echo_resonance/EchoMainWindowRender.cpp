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
//  主场景渲染（标题 → 玩家选择）
// ═══════════════════════════════════════════

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
    renderSpiritAura();
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
