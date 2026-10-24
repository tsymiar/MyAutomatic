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
    // ── 离奇魔幻基因：声灵系统信号 → 漂浮文字提示 ──
    connect(m_engine, &EchoEngine::spiritManifested, this, [this](SpiritKind kind) {
        auto* s = m_engine->findSpirit(kind);
        if (!s) return;
        FloatingNoise fn; fn.text = QString("声灵显形：%1·%2").arg(s->name, s->title);
        fn.x = -0.35f; fn.y = 0.35f; fn.alpha = 0.95f; fn.life = 4.0f;
        m_noiseTexts.push_back(fn);
        });
    connect(m_engine, &EchoEngine::spiritBacklash, this, [this](SpiritKind kind) {
        FloatingNoise fn; fn.text = m_engine->spiritBacklashText(kind);
        fn.x = -0.3f; fn.y = 0.55f; fn.alpha = 1.0f; fn.life = 5.0f;
        m_noiseTexts.push_back(fn);
        });
    connect(m_engine, &EchoEngine::primeEchoOverloaded, this, [this]() {
        FloatingNoise fn; fn.text = "原初嗡鸣过载——真相在崩坏的表象下显影。";
        fn.x = -0.45f; fn.y = 0.65f; fn.alpha = 1.0f; fn.life = 6.0f;
        m_noiseTexts.push_back(fn);
        });
    connect(m_engine, &EchoEngine::soulFragmentCollected, this, [this](int id) {
        const auto& frags = m_engine->soulFragments();
        if (id < 0 || id >= int(frags.size())) return;
        FloatingNoise fn; fn.text = QString("灵魂碎片「%1」  %2/%3").arg(frags[id].name)
            .arg(m_engine->collectedSoulFragmentCount()).arg(int(frags.size()));
        fn.x = 0.0f; fn.y = 0.2f; fn.alpha = 0.95f; fn.life = 4.0f;
        m_noiseTexts.push_back(fn);
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
//  切换选项
// ═══════════════════════════════════════════

void EchoMainWindow::showChapterIntro()
{
    m_prevState = m_state;
    m_chapterIntroAlpha = 0.0f;
    m_chapterIntroShown = false;
    m_state = GameState::ChapterIntro;
    // 章节推进时揭示对应角色（接入 revealCharacter）
    // 0=玩家 1=林薇 2=陈远山 3=老刘 4=何悦 5=底噪
    GameChapter ch = m_engine->currentChapter();
    switch (ch) {
    case GameChapter::Prologue: m_engine->revealCharacter(0); m_engine->revealCharacter(1); break; // 玩家+林薇
    case GameChapter::Chapter2: m_engine->revealCharacter(2); m_engine->revealCharacter(3); break; // 陈远山+老刘
    case GameChapter::Chapter3: m_engine->revealCharacter(4); break; // 何悦
    case GameChapter::Chapter4: m_engine->revealCharacter(5); break; // 底噪
    default: break;
    }
}

void EchoMainWindow::showChoice(const QString& prompt, const QStringList& options)
{
    m_choicePrompt = prompt; m_choiceOptions = options; m_choiceSelectedIndex = 0;
    m_prevState = m_state; m_state = GameState::PlayerChoice;
}

// ═══════════════════════════════════════════
//  坐标换算与命中测试
// ═══════════════════════════════════════════

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
                // 进入终章：在真相面前拼出被篡改的人生
                m_engine->generateFragments(GameChapter::Finale);
                showChapterIntro();
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
