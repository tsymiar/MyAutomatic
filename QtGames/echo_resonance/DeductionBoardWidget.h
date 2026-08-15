#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include "DeductionData.h"
#include "DeductionEngine.h"
#include "DeductionAudio.h"
#include <vector>

// ═══════════════════════════════════════════════════════════════
//  声纹推演盘 —— 主界面
//  三槽拖拽（样本+动作+锚点）+ 心理变量滑块 + 命题展示
// ═══════════════════════════════════════════════════════════════
class DeductionBoardWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit DeductionBoardWidget(QWidget* parent = nullptr);
    ~DeductionBoardWidget();

    // 设置章节（0=序章 1=Ch1 2=Ch2 3=Ch3 4=终章）
    void loadChapter(int chapter);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private slots:
    void tick();

private:
    void renderSamples();       // 样本库（左侧）
    void renderBoard();         // 推演盘三槽（中部）
    void renderProposition();   // 命题 + 心理变量滑块（顶部/右侧）
    void renderResult();        // 推演结果
    void renderPersonality();   // 人格侧写（终章）

    // 辅助
    void drawTextGL(float x, float y, const QString& t, const QColor& c, float s);
    void drawRectGL(float x, float y, float w, float h, const QColor& c, bool filled);
    void drawCircleGL(float cx, float cy, float r, const QColor& c, int seg = 32);
    QPointF screenToGL(const QPoint& p) const;
    int hitSample(const QPointF& gp) const;
    int hitSlot(const QPointF& gp) const;

    // 执行推演
    void performDeduction();

    DeductionEngine m_engine;
    DeductionAudio m_audio;

    // 推演盘状态
    std::vector<DeductionSlot> m_slots;    // 三个槽
    std::vector<Relation> m_relations;     // 关系
    int m_selectedSample = -1;             // 选中的样本（拖拽中）
    bool m_dragging = false;
    QPointF m_dragPos;
    DeductionResult m_lastResult;
    bool m_hasResult = false;

    // 心理变量滑块
    float m_greed = 0.5f;   // 贪婪
    float m_fear = 0.5f;    // 恐惧
    bool m_draggingGreed = false;
    bool m_draggingFear = false;

    // 关系选择
    int m_selectedRelation = 0;

    // 时间
    QTimer* m_timer;
    QElapsedTimer m_elapsed;
    float m_gameTime = 0.0f;

    // 窗口
    int m_winW = 800, m_winH = 600;

    // 章节
    int m_chapter = 0;
};
