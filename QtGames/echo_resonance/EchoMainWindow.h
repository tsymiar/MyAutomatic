#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointF>
#include "EchoEngine.h"
#include <vector>
#include <deque>

class DeductionBoardWidget;  // 声纹推演盘

// ── 游戏渲染状态 ──
enum class GameState {
    Title,           // 标题画面
    NameInput,       // 角色名输入
    Tutorial,        // 序章教程
    FragmentSelect,  // 碎片选择
    TimelinePuzzle,  // 时间轴拼图
    SceneComplete,   // 场景完成
    MentorMessage,   // 导师信息
    PlayerChoice,    // 玩家选择界面
    ChapterIntro,    // 章节介绍
    EchoArchive,     // 回声碎片档案（支线）
    EndingScreen,    // 结局画面
    EpilogueScreen,  // 结局余波录音
    GameOver         // 多周目提示
};

// ── 视觉粒子 ──
struct GlitchParticle { float x, y, vx, vy, life, maxLife; QColor color; };
struct SpectrumLine { float frequency, amplitude, phase; QColor color; };
struct HallucinationText { QString text; float x, y, alpha, speedX, speedY, life; };
struct FloatingNoise { QString text; float x, y, alpha, life; }; // 底噪对话

class EchoMainWindow : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit EchoMainWindow(QWidget* parent = nullptr);
    ~EchoMainWindow();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private slots:
    void gameTick();
    void onFragmentPlaced(int fragmentId, int slotIndex);
    void onFragmentRemoved(int fragmentId, int slotIndex);
    void onSceneCompleted(int chapter, bool isDistorted);
    void onParanoiaChanged(float level);
    void onAnxietyChanged(float level);
    void onHallucinationTriggered(const QString& message);
    void onReverseUnlocked();
    void onNoiseCommunion(const QString& message);

private:
    // ── 渲染 ──
    void renderTitle();
    void renderNameInput();
    void renderTutorial();
    void renderFragmentSelect();
    void renderTimelinePuzzle();
    void renderSceneComplete();
    void renderMentorMessage();
    void renderPlayerChoice();
    void renderChapterIntro();
    void renderEchoArchive();
    void renderEndingScreen();
    void renderEpilogueScreen();
    void renderResonanceAura();
    void renderSilencePunishment();   // 右耳失聪视觉
    void renderFragmentEchoes();      // 衰减回声波纹
    void renderSilenceMorse();        // 老刘静默摩斯
    void renderGlitchEffects();
    void renderParanoiaMeter();
    void renderAnxietyMeter();
    void renderSpectrumVisualization();
    void renderHallucinations();
    void renderNoiseDialogues();
    void renderSilenceOverlay();
    void renderReverseIndicator();
    void renderMorseIndicator();
    void renderTinnitusGuide();  // 耳鸣指引（林薇的潜意识引导）

    // ── 绘制辅助 ──
    void drawTextGL(float x, float y, const QString& text, const QColor& color, float scale = 1.0f);
    void drawRectGL(float x, float y, float w, float h, const QColor& color, bool filled = true);
    void drawLineGL(float x1, float y1, float x2, float y2, const QColor& color, float width = 1.0f);
    void drawCircleGL(float cx, float cy, float r, const QColor& color, int segments = 32);
    QPointF screenToGL(const QPoint& screenPos) const;
    int hitTestFragment(const QPointF& glPos) const;
    int hitTestTimelineSlot(const QPointF& glPos) const;

    // ── 选择界面辅助 ──
    void showChoice(const QString& prompt, const QStringList& options);
    void showChapterIntro();

    // ── 引擎 ──
    EchoEngine* m_engine;
    GameState m_state = GameState::Title;
    GameState m_prevState = GameState::Title;
    int m_selectedFragmentIndex = -1;
    bool m_isDragging = false;
    QPointF m_dragStartPos, m_dragCurrentPos;

    // ── 计时 ──
    QTimer* m_tickTimer;
    QElapsedTimer m_elapsed;
    float m_gameTime = 0.0f;

    // ── 视觉效果 ──
    std::vector<GlitchParticle> m_glitchParticles;
    std::vector<SpectrumLine> m_spectrumLines;
    std::vector<HallucinationText> m_hallucinations;
    std::vector<FloatingNoise> m_noiseTexts;
    float m_glitchIntensity = 0.0f;
    float m_noiseOffset = 0.0f;
    float m_paranoiaDisplay = 0.0f;
    float m_anxietyDisplay = 0.0f;
    float m_bgFlicker = 0.0f;
    float m_lowFreqDisplay = 0.0f;
    float m_silenceAlpha = 0.0f;
    bool m_silenceMode = false;

    // ── 导师/输入 ──
    float m_mentorMessageAlpha = 0.0f;
    QString m_currentMentorMessage;
    int m_mentorMessageIndex = 0;
    bool m_showMentorInput = false;
    QString m_playerReply;
    QString m_playerName;

    // ── 选择界面 ──
    QString m_choicePrompt;
    QStringList m_choiceOptions;
    int m_choiceSelectedIndex = 0;

    // ── 章节介绍 ──
    float m_chapterIntroAlpha = 0.0f;
    bool m_chapterIntroShown = false;

    // ── 回声碎片档案 ──
    int m_echoArchiveIndex = 0;
    float m_echoArchiveScroll = 0.0f;

    // ── 结局余波 ──
    QString m_epilogueText;
    float m_epilogueAlpha = 0.0f;
    float m_epilogueTimer = 0.0f;
    bool m_epilogueFinished = false;

    // ── 共鸣度 ──
    float m_resonanceDisplay = 0.0f;
    float m_resonanceAuraPhase = 0.0f;

    // ── 巧妙机制显示状态 ──
    float m_idleTime = 0.0f;            // 玩家静默时长（无操作）
    bool m_silenceMorseShown = false;   // 老刘静默摩斯是否已展示
    QString m_silenceMorseText;

    // ── 耳鸣指引（林薇潜意识引导）──
    float m_stuckTime = 0.0f;           // 当前场景停留时长
    bool m_tinnitusActive = false;      // 耳鸣指引是否激活
    float m_tinnitusPhase = 0.0f;       // 耳鸣波纹相位

    // ── 声纹推演盘（--echo 统一入口，从标题画面进入）──
    DeductionBoardWidget* m_deductionBoard = nullptr;

    // ── 标题动画 ──
    float m_titlePulse = 0.0f;
    float m_titleWaveOffset = 0.0f;
    float m_fragmentScrollY = 0.0f;
    int m_winWidth = 800, m_winHeight = 600;

    // ── 布局常量 ──
    static constexpr float TIMELINE_X = -0.95f, TIMELINE_Y = -0.55f;
    static constexpr float TIMELINE_W = 1.9f, TIMELINE_H = 0.35f;
    static constexpr float SLOT_W = 0.08f, SLOT_GAP = 0.02f;
};
