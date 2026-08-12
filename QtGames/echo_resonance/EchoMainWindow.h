#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointF>
#include "EchoEngine.h"
#include <vector>
#include <deque>

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
    EndingScreen,    // 结局画面
    GameOver         // 多周目提示
};

// ── 视觉粒子 ──
struct GlitchParticle { float x,y,vx,vy,life,maxLife; QColor color; };
struct SpectrumLine  { float frequency,amplitude,phase; QColor color; };
struct HallucinationText { QString text; float x,y,alpha,speedX,speedY,life; };
struct FloatingNoise  { QString text; float x,y,alpha,life; }; // 底噪对话

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
    void renderEndingScreen();
    void renderGlitchEffects();
    void renderParanoiaMeter();
    void renderAnxietyMeter();
    void renderSpectrumVisualization();
    void renderHallucinations();
    void renderNoiseDialogues();
    void renderSilenceOverlay();
    void renderReverseIndicator();
    void renderMorseIndicator();

    // ── 绘制辅助 ──
    void drawTextGL(float x, float y, const QString& text, const QColor& color, float scale=1.0f);
    void drawRectGL(float x, float y, float w, float h, const QColor& color, bool filled=true);
    void drawLineGL(float x1, float y1, float x2, float y2, const QColor& color, float width=1.0f);
    void drawCircleGL(float cx, float cy, float r, const QColor& color, int segments=32);
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
