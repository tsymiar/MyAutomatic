#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include "EchoEngine.h"

enum class GameState {
    Title,          // 标题画面
    NameInput,      // 角色名输入
    MainMenu,       // 主菜单（游戏入口，待扩展）
    GameOver        // 退出提示
};

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

private slots:
    void gameTick();

private:
    void renderTitle();
    void renderNameInput();
    void renderMainMenu();

    void drawTextGL(float x, float y, const QString& text, const QColor& color, float scale = 1.0f);
    void drawRectGL(float x, float y, float w, float h, const QColor& color, bool filled = true);
    void drawLineGL(float x1, float y1, float x2, float y2, const QColor& color, float width = 1.0f);
    void drawCircleGL(float cx, float cy, float r, const QColor& color, int segments = 32);

    EchoEngine* m_engine;
    GameState m_state = GameState::Title;
    QTimer* m_tickTimer;
    QElapsedTimer m_elapsed;
    float m_gameTime = 0.0f;
    QString m_playerName;
    int m_winWidth = 800;
    int m_winHeight = 600;
};
