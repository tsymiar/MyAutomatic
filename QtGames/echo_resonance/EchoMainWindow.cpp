#include "EchoMainWindow.h"
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

EchoMainWindow::EchoMainWindow(QWidget* parent) : QOpenGLWidget(parent) {
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
        SpectrumLine l; l.frequency=20.0f*powf(2.0f,i/8.0f);
        l.amplitude=(rand()%100)/200.0f; l.phase=(rand()%628)/100.0f;
        l.color=QColor(0,180,255,60); m_spectrumLines.push_back(l);
    }
}

EchoMainWindow::~EchoMainWindow() {
    if (m_tickTimer && m_tickTimer->isActive()) m_tickTimer->stop();
}

// ═══════════════════════════════════════════
//  OpenGL 初始化
// ═══════════════════════════════════════════

void EchoMainWindow::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.05f,0.05f,0.08f,1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
}

void EchoMainWindow::resizeGL(int w, int h) {
    m_winWidth=w; m_winHeight=h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    float aspect=float(w)/float(h?h:1);
    if(aspect>1.0f) glOrtho(-aspect,aspect,-1.0,1.0,-1.0,1.0);
    else glOrtho(-1.0,1.0,-1.0/aspect,1.0/aspect,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ═══════════════════════════════════════════
//  游戏循环
// ═══════════════════════════════════════════

void EchoMainWindow::gameTick() {
    float dt=m_elapsed.elapsed()/1000.0f; m_elapsed.restart();
    if(dt>0.1f) dt=0.1f; m_gameTime+=dt;
    m_titlePulse=1.0f+0.08f*sinf(m_gameTime*2.5f);
    m_titleWaveOffset+=dt*0.8f;
    if(m_glitchIntensity>0.0f){m_glitchIntensity-=dt*0.3f;if(m_glitchIntensity<0.0f)m_glitchIntensity=0.0f;}
    m_noiseOffset+=dt*15.0f;
    m_bgFlicker=0.02f*sinf(m_gameTime*3.7f)+0.01f*sinf(m_gameTime*7.3f);
    float tp=m_engine->paranoiaLevel(); m_paranoiaDisplay+=(tp-m_paranoiaDisplay)*dt*3.0f;
    float ta=m_engine->anxietyLevel(); m_anxietyDisplay+=(ta-m_anxietyDisplay)*dt*3.0f;
    float tl=m_engine->lowFreqIntensity(); m_lowFreqDisplay+=(tl-m_lowFreqDisplay)*dt*2.0f;
    // 静默模式衰减
    if(m_silenceMode && m_silenceAlpha<0.9f) m_silenceAlpha+=dt*2.0f;
    if(!m_silenceMode && m_silenceAlpha>0.0f) m_silenceAlpha-=dt*1.5f;
    if(m_silenceAlpha<0.0f)m_silenceAlpha=0.0f;
    for(auto& l:m_spectrumLines){l.phase+=dt*l.frequency*0.1f; float ta=(rand()%100)/200.0f; l.amplitude+=(ta-l.amplitude)*dt*2.0f;}
    for(auto it=m_glitchParticles.begin();it!=m_glitchParticles.end();){
        it->x+=it->vx*dt;it->y+=it->vy*dt;it->life-=dt;
        if(it->life<=0.0f) it=m_glitchParticles.erase(it); else ++it;
    }
    if(m_engine->isDistorted()&&m_state==GameState::TimelinePuzzle){
        m_glitchIntensity=std::min(1.0f,m_glitchIntensity+dt*0.5f);
        if(rand()%100<40){GlitchParticle p;p.x=(rand()%200-100)/100.0f;p.y=(rand()%200-100)/100.0f;p.vx=(rand()%100-50)/200.0f;p.vy=(rand()%100-50)/200.0f;p.life=0.5f+(rand()%50)/100.0f;p.maxLife=p.life;p.color=QColor(255,rand()%100,rand()%50,100);m_glitchParticles.push_back(p);}
    }
    for(auto it=m_hallucinations.begin();it!=m_hallucinations.end();){
        it->x+=it->speedX*dt;it->y+=it->speedY*dt;it->life-=dt;it->alpha=std::min(it->alpha,it->life/3.0f);
        if(it->life<=0.0f||it->alpha<=0.0f) it=m_hallucinations.erase(it); else ++it;
    }
    for(auto it=m_noiseTexts.begin();it!=m_noiseTexts.end();){
        it->life-=dt;it->alpha=std::min(it->alpha,it->life/2.0f);
        if(it->life<=0.0f||it->alpha<=0.0f) it=m_noiseTexts.erase(it); else ++it;
    }
    if(m_state==GameState::MentorMessage&&m_mentorMessageAlpha<1.0f){m_mentorMessageAlpha+=dt*0.5f;if(m_mentorMessageAlpha>1.0f)m_mentorMessageAlpha=1.0f;}
    if(m_state==GameState::ChapterIntro&&m_chapterIntroAlpha<1.0f){m_chapterIntroAlpha+=dt*0.8f;if(m_chapterIntroAlpha>1.0f)m_chapterIntroAlpha=1.0f;}
    update();
}

// ═══════════════════════════════════════════
//  渲染主入口
// ═══════════════════════════════════════════

void EchoMainWindow::paintGL() {
    float bgR=0.05f+m_bgFlicker,bgG=0.05f+m_bgFlicker*0.5f,bgB=0.08f+m_bgFlicker;
    glClearColor(bgR,bgG,bgB,1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    renderSpectrumVisualization();
    if(m_glitchIntensity>0.01f) renderGlitchEffects();
    renderHallucinations();
    renderNoiseDialogues();
    switch(m_state){
    case GameState::Title: renderTitle(); break;
    case GameState::NameInput: renderNameInput(); break;
    case GameState::Tutorial: renderTutorial(); break;
    case GameState::FragmentSelect: renderFragmentSelect(); break;
    case GameState::TimelinePuzzle: renderTimelinePuzzle(); break;
    case GameState::SceneComplete: renderSceneComplete(); break;
    case GameState::MentorMessage: renderMentorMessage(); break;
    case GameState::PlayerChoice: renderPlayerChoice(); break;
    case GameState::ChapterIntro: renderChapterIntro(); break;
    case GameState::EndingScreen: renderEndingScreen(); break;
    case GameState::GameOver: renderTitle(); break;
    }
    renderReverseIndicator();
    renderMorseIndicator();
    renderParanoiaMeter();
    renderAnxietyMeter();
    if(m_silenceAlpha>0.01f) renderSilenceOverlay();
}

// ═══════════════════════════════════════════
//  标题画面
// ═══════════════════════════════════════════

void EchoMainWindow::renderTitle() {
    for(int i=0;i<5;++i){float r=0.3f+i*0.12f+0.02f*sinf(m_gameTime*1.5f+i),a=0.15f-i*0.025f;drawCircleGL(0,0.1f,r,QColor(0,150,255,int(a*255)),64);}
    const QString title="余音回响"; float tw=title.length()*0.12f,sx=-tw/2.0f;
    for(int i=0;i<title.length();++i){float p=1.0f+0.05f*sinf(m_gameTime*3.0f+i*1.2f),x=sx+i*0.12f,y=0.15f+0.01f*sinf(m_gameTime*2.0f+i*0.8f);int a=200+55*sinf(m_gameTime*2.5f+i);drawTextGL(x,y,QString(title[i]),QColor(0,200,255,a),p*1.8f);}
    drawTextGL(-0.3f,-0.08f,"Echo Resonance",QColor(100,180,220,150),0.9f);
    float ly=-0.15f,la=0.3f+0.15f*sinf(m_gameTime*2.0f);drawLineGL(-0.4f,ly,0.4f,ly,QColor(0,180,255,int(la*255)),1.5f);
    if(sinf(m_gameTime*3.0f)>-0.3f) drawTextGL(-0.25f,-0.35f,"按 ENTER 开始",QColor(180,200,220,200),0.7f);
    drawTextGL(-0.4f,-0.55f,"操作：鼠标拖拽碎片到时间轴 | ESC 退出 | R 重置",QColor(100,120,140,120),0.45f);
    for(int i=0;i<200;++i){float t=i/200.0f,x=-0.95f+t*1.9f,y=-0.85f+0.03f*sinf(t*30.0f+m_gameTime*4.0f)*(1.0f+0.5f*sinf(m_gameTime));
        if(i>0){float t2=(i-1)/200.0f,x2=-0.95f+t2*1.9f,y2=-0.85f+0.03f*sinf(t2*30.0f+m_gameTime*4.0f)*(1.0f+0.5f*sinf(m_gameTime));drawLineGL(x2,y2,x,y,QColor(255,40,40,30),0.5f);}}
}

// ═══════════════════════════════════════════
//  角色名输入
// ═══════════════════════════════════════════

void EchoMainWindow::renderNameInput() {
    for(int i=0;i<4;++i){float r=0.25f+i*0.15f+0.02f*sinf(m_gameTime*1.5f+i),a=0.12f-i*0.025f;drawCircleGL(0,0.05f,r,QColor(0,150,255,int(a*255)),64);}
    drawTextGL(-0.35f,0.55f,"你的名字",QColor(0,200,255,220),1.2f);
    drawTextGL(-0.45f,0.35f,"请输入你在游戏中的角色名",QColor(150,180,200,180),0.55f);
    drawTextGL(-0.55f,0.25f,"导师会用这个名字称呼你，这会影响你看到的叙事内容",QColor(100,130,160,150),0.4f);
    drawRectGL(-0.35f,0.05f,0.7f,0.1f,QColor(10,20,40,180),true);
    drawRectGL(-0.35f,0.05f,0.7f,0.1f,QColor(0,180,255,80),false);
    QString dn=m_playerName;
    if(int(m_gameTime*2)%2==0) dn+="|";
    if(dn.isEmpty()||(dn=="|"&&m_playerName.isEmpty())) drawTextGL(-0.3f,0.09f,"输入你的名字...",QColor(60,90,130,100),0.55f);
    else drawTextGL(-0.3f,0.09f,dn,QColor(0,255,200,220),0.6f);
    float b=0.6f+0.4f*sinf(m_gameTime*3.0f);
    drawTextGL(-0.3f,-0.15f,"按 ENTER 确认",QColor(0,200,255,int(b*200)),0.55f);
    drawTextGL(-0.2f,-0.3f,"ESC: 返回标题 | 留空则使用默认名\"小周\"",QColor(100,120,140,120),0.4f);
    for(int i=0;i<200;++i){float t=i/200.0f,x=-0.95f+t*1.9f,y=-0.85f+0.02f*sinf(t*25.0f+m_gameTime*3.0f);
        if(i>0){float t2=(i-1)/200.0f,x2=-0.95f+t2*1.9f,y2=-0.85f+0.02f*sinf(t2*25.0f+m_gameTime*3.0f);drawLineGL(x2,y2,x,y,QColor(255,40,40,20),0.3f);}}
}

// ═══════════════════════════════════════════
//  教程界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderTutorial() {
    drawTextGL(-0.9f,0.85f,"序章：入职第7天 — 教程",QColor(0,200,255,220),1.0f);
    drawTextGL(-0.9f,0.72f,"目标：还原昨天下午茶水间的对话",QColor(150,200,220,180),0.5f);
    drawTextGL(-0.9f,0.65f,"操作：点击左侧碎片 → 拖放到时间轴槽位 → 按 TAB 切换到时间轴",QColor(120,160,200,180),0.4f);
    drawTextGL(-0.9f,0.58f,"蓝色碎片=真实残留 | 青色=可疑 | 黄色=噪声 | 红色=伪造",QColor(100,140,180,150),0.38f);
    // 锚点提示
    AnchorType a=m_engine->currentAnchor();
    QString an="锚点音：";
    switch(a){case AnchorType::DoorBeep:an+="门禁刷卡声（必须放在第1个槽位）";break;
    case AnchorType::CoffeeMachine:an+="咖啡机启动声";break;
    case AnchorType::HeartMonitor:an+="心率监护仪";break;
    default:an+="无";break;}
    drawTextGL(-0.9f,-0.85f,an,QColor(255,200,0,180),0.4f);
    drawTextGL(0.1f,-0.85f,"按 TAB 开始拼接 | ESC 退出",QColor(100,120,140,120),0.4f);
}

// ═══════════════════════════════════════════
//  章节介绍
// ═══════════════════════════════════════════

void EchoMainWindow::renderChapterIntro() {
    float a=m_chapterIntroAlpha;
    glClearColor(0.02f*a,0.02f*a,0.04f*a,1.0f); glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.05f,0.05f,0.08f,1.0f);
    drawTextGL(-0.6f,0.5f,m_engine->chapterName(),QColor(0,200,255,int(a*220)),1.3f);
    QString desc=m_engine->chapterDescription();
    drawTextGL(-0.8f,0.2f,desc,QColor(180,200,220,int(a*180)),0.42f);
    if(a>0.8f){float b=0.6f+0.4f*sinf(m_gameTime*3.0f);drawTextGL(-0.3f,-0.5f,"按 ENTER 继续",QColor(0,200,255,int(b*200)),0.6f);}
}

void EchoMainWindow::showChapterIntro() {
    m_prevState=m_state;
    m_chapterIntroAlpha=0.0f;
    m_chapterIntroShown=false;
    m_state=GameState::ChapterIntro;
}

// ═══════════════════════════════════════════
//  碎片选择界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderFragmentSelect() {
    drawTextGL(-0.9f,0.85f,m_engine->chapterName(),QColor(0,200,255,220),1.1f);
    QString desc=m_engine->chapterDescription();
    QString w;int cpl=55;
    for(int i=0;i<desc.length();i+=cpl){w+=desc.mid(i,cpl);if(i+cpl<desc.length()) w+="\n";}
    drawTextGL(-0.9f,0.72f,w,QColor(150,180,200,180),0.4f);
    float px=-0.9f,py=0.55f,pw=0.85f,ih=0.11f,gap=0.02f;
    auto& frags=m_engine->fragments();
    int tf=static_cast<int>(frags.size());
    float so=m_fragmentScrollY;
    int vs=std::max(0,int(-so/(ih+gap))),ve=std::min(tf,vs+12);
    for(int i=vs;i<ve;++i){
        auto& f=frags[i]; if(f.isPlaced) continue;
        float y=py+(ih+gap)*i+so; if(y<-1.0f||y>1.0f) continue;
        QColor bc=f.displayColor(); bc.setAlpha(40); drawRectGL(px,y-ih,pw,ih,bc,true);
        bool sel=(m_selectedFragmentIndex==i);
        QColor boc=f.displayColor(); boc.setAlpha(sel?255:80); drawRectGL(px,y-ih,pw,ih,boc,false);
        drawTextGL(px+0.02f,y-0.02f,f.name,f.displayColor(),0.55f);
        drawTextGL(px+0.02f,y-0.06f,f.credibilityLabel(),f.displayColor().lighter(130),0.35f);
        if(f.isAnchor){drawTextGL(px+0.02f,y-0.09f,"[锚点音]",QColor(255,200,0,180),0.3f);}
        QString d=QString("%1秒").arg(f.durationSec,0,'f',1);
        drawTextGL(px+pw-0.12f,y-0.04f,d,QColor(150,150,150,150),0.35f);
    }
    QString cs=QString("可用碎片: %1 / %2 (已放置: %3)").arg(tf-m_engine->placedFragmentCount()).arg(tf).arg(m_engine->placedFragmentCount());
    drawTextGL(-0.9f,-0.9f,cs,QColor(120,140,160,150),0.4f);
    if(m_selectedFragmentIndex>=0&&m_selectedFragmentIndex<tf){
        auto& f=frags[m_selectedFragmentIndex];
        drawTextGL(0.1f,0.55f,"已选中:",QColor(0,200,255,200),0.6f);
        drawTextGL(0.1f,0.48f,f.name,f.displayColor(),0.7f);
        drawTextGL(0.1f,0.42f,f.description,QColor(150,180,200,180),0.35f);
        drawTextGL(0.1f,0.36f,QString("可信度: %1").arg(f.credibilityLabel()),f.displayColor(),0.4f);
        if(f.isReversed) drawTextGL(0.1f,0.30f,"[已逆向]",QColor(255,100,200,200),0.4f);
    }
    drawTextGL(0.1f,-0.6f,"ESC: 返回 | TAB: 时间轴 | R: 逆向播放选中碎片",QColor(100,120,140,120),0.4f);
}

// ═══════════════════════════════════════════
//  时间轴拼图
// ═══════════════════════════════════════════

void EchoMainWindow::renderTimelinePuzzle() {
    drawTextGL(-0.9f,0.85f,m_engine->chapterName(),QColor(0,200,255,220),0.9f);
    float coh=m_engine->calculateCoherence();
    QString cs=QString("场景一致性: %1%").arg(int(coh*100));
    QColor cc=coh>0.7f?QColor(0,200,100):(coh>0.4f?QColor(200,200,0):QColor(255,60,60));
    drawTextGL(0.2f,0.85f,cs,cc,0.55f);
    if(m_engine->isDistorted()){float a=0.5f+0.3f*sinf(m_gameTime*6.0f);drawTextGL(0.2f,0.78f,"⚠ 声纹冲突 - 场景已扭曲",QColor(255,80,40,int(a*255)),0.5f);}
    auto& frags=m_engine->fragments();
    float pnx=-0.95f,pny=0.65f,iw=0.25f,ih=0.06f,gap=0.01f; int col=0,row=0,mc=3;
    for(size_t i=0;i<frags.size();++i){
        auto& f=frags[i]; if(f.isPlaced) continue;
        float x=pnx+col*(iw+gap),y=pny-row*(ih+gap*3);
        QColor bc=f.displayColor(); bc.setAlpha(m_selectedFragmentIndex==int(i)?60:25); drawRectGL(x,y-ih,iw,ih,bc,true);
        QColor boc=f.displayColor(); boc.setAlpha(m_selectedFragmentIndex==int(i)?255:60); drawRectGL(x,y-ih,iw,ih,boc,false);
        drawTextGL(x+0.01f,y-0.02f,f.name,f.displayColor(),0.4f);
        drawTextGL(x+0.01f,y-0.05f,f.credibilityLabel(),f.displayColor().lighter(130),0.28f);
        col++; if(col>=mc){col=0;row++;}
    }
    float tlx=TIMELINE_X,tly=TIMELINE_Y,tlw=TIMELINE_W,tlh=TIMELINE_H;
    int sc=m_engine->timelineSlotCount();
    float sw=std::min(0.1f,(tlw-(sc-1)*SLOT_GAP)/sc);
    float tsw=sc*sw+(sc-1)*SLOT_GAP,ssx=tlx+(tlw-tsw)/2.0f;
    drawRectGL(tlx,tly,tlw,tlh,QColor(10,15,25,180),true);
    drawRectGL(tlx,tly,tlw,tlh,QColor(0,150,200,60),false);
    drawTextGL(tlx,tly+tlh+0.02f,"时间轴 (拖放碎片到槽位)",QColor(100,160,200,180),0.35f);
    for(int i=0;i<sc;++i){
        float sx=ssx+i*(sw+SLOT_GAP),sy=tly+0.02f,sh=tlh-0.04f;
        int fid=m_engine->timeline()[i].fragmentId;
        if(fid>=0){for(auto& f:frags)if(f.id==fid){QColor sc=f.displayColor();sc.setAlpha(80);drawRectGL(sx,sy,sw,sh,sc,true);drawRectGL(sx,sy,sw,sh,f.displayColor(),false);drawTextGL(sx+0.005f,sy+sh/2+0.01f,f.name,f.displayColor(),0.3f);break;}}
        else{drawRectGL(sx,sy,sw,sh,QColor(30,40,60,60),true);drawRectGL(sx,sy,sw,sh,QColor(60,80,120,40),false);
            QString label=QString::number(i+1);
            if(m_engine->timeline()[i].isAnchorSlot) label="[锚]"+label;
            drawTextGL(sx+0.005f,sy+sh/2+0.01f,label,QColor(60,80,120,80),0.25f);}
    }
    if(m_isDragging&&m_selectedFragmentIndex>=0){
        auto& f=frags[m_selectedFragmentIndex];
        QPointF gp=screenToGL(QPoint(int(m_dragCurrentPos.x()),int(m_dragCurrentPos.y())));
        QColor dc=f.displayColor();dc.setAlpha(120);drawRectGL(gp.x()-0.04f,gp.y()-0.03f,0.08f,0.06f,dc,true);drawTextGL(gp.x()-0.03f,gp.y(),f.name,f.displayColor(),0.35f);
    }
    drawTextGL(-0.9f,-0.9f,"ESC: 返回碎片选择 | R: 重置 | 右键槽位移除 | TAB: 逆向播放选中",QColor(100,120,140,120),0.35f);
}

// ═══════════════════════════════════════════
//  场景完成
// ═══════════════════════════════════════════

void EchoMainWindow::renderSceneComplete() {
    bool d=m_engine->isDistorted();
    if(d){glClearColor(0.12f,0.03f,0.03f,1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f,0.05f,0.08f,1.0f);
        for(int i=0;i<200;++i){float x=(rand()%200-100)/100.0f,y=(rand()%200-100)/100.0f,s=(rand()%5)/500.0f;drawRectGL(x,y,s,s,QColor(255,rand()%60,rand()%60,rand()%60),true);}}
    else{for(int i=0;i<3;++i){float r=0.4f+i*0.15f,a=0.08f-i*0.02f;drawCircleGL(0,0.2f,r,QColor(0,150,255,int(a*255)),64);}}
    QString tt=d?"场景重建 - 扭曲版本":"场景重建完成";
    drawTextGL(-0.4f,0.5f,tt,d?QColor(255,80,40):QColor(0,200,255),1.2f);
    float coh=m_engine->calculateCoherence();
    drawTextGL(-0.3f,0.3f,QString("声纹一致性: %1%").arg(int(coh*100)),d?QColor(255,120,60):QColor(0,200,150),0.7f);
    QString desc=d?"你使用了冲突的声纹碎片强行拼接。\n现实出现了裂痕——但裂痕中，有时会透出真相。":"音频场景完整重现。但请记住导师的警告：别相信你听到的任何声音。";
    drawTextGL(-0.5f,0.1f,desc,QColor(180,200,220,200),0.45f);
    if(d||m_engine->paranoiaLevel()>0.5f){float a=0.5f+0.3f*sinf(m_gameTime*2.0f);QString msg=m_engine->getMentorMessage(m_mentorMessageIndex);drawTextGL(-0.5f,-0.2f,"\""+msg+"\"",QColor(255,200,100,int(a*255)),0.55f);}
    drawTextGL(-0.3f,-0.5f,"ENTER: 继续 | R: 重新拼接",QColor(150,180,200,180),0.5f);
    if(d){float b=sinf(m_gameTime*4.0f)>0?1.0f:0.3f;drawTextGL(-0.2f,-0.6f,"提示：扭曲版本可能揭示了隐藏线索",QColor(255,80,40,int(b*200)),0.4f);}
}

// ═══════════════════════════════════════════
//  导师信息
// ═══════════════════════════════════════════

void EchoMainWindow::renderMentorMessage() {
    float a=m_mentorMessageAlpha;
    glClearColor(0.02f*a,0.02f*a,0.04f*a,1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f,0.05f,0.08f,1.0f);
    for(int i=0;i<4;++i){float r=0.2f+i*0.18f+0.03f*sinf(m_gameTime+i);drawCircleGL(0,0.15f,r,QColor(0,150,255,int(a*40-i*8)),64);}
    drawTextGL(-0.5f,0.5f,"导师的语音日志 #"+QString::number(m_mentorMessageIndex+1),QColor(0,200,255,int(a*220)),0.8f);
    drawTextGL(-0.6f,0.2f,"\""+m_currentMentorMessage+"\"",QColor(200,220,255,int(a*200)),0.55f);
    if(m_showMentorInput){float b=0.6f+0.4f*sinf(m_gameTime*3.0f);drawTextGL(-0.4f,-0.2f,"请输入你的回复（按 ENTER 发送）:",QColor(0,200,255,int(a*b*255)),0.5f);
        QString dr=m_playerReply; if(int(m_gameTime*2)%2==0) dr+="|"; drawTextGL(-0.4f,-0.35f,dr,QColor(0,255,200,int(a*220)),0.6f);}
    else drawTextGL(-0.3f,-0.3f,"按 ENTER 继续",QColor(150,180,200,int(a*150)),0.5f);
    for(int i=0;i<300;++i){float t=i/300.0f,x=-0.95f+t*1.9f,y=-0.7f+0.02f*sinf(t*50.0f+m_gameTime*3.0f);
        if(i>0){float t2=(i-1)/300.0f,x2=-0.95f+t2*1.9f,y2=-0.7f+0.02f*sinf(t2*50.0f+m_gameTime*3.0f);drawLineGL(x2,y2,x,y,QColor(255,40,40,int(a*20)),0.3f);}}
}

// ═══════════════════════════════════════════
//  玩家选择界面
// ═══════════════════════════════════════════

void EchoMainWindow::renderPlayerChoice() {
    glClearColor(0.03f,0.03f,0.06f,1.0f);glClear(GL_COLOR_BUFFER_BIT);glClearColor(0.05f,0.05f,0.08f,1.0f);
    drawTextGL(-0.7f,0.7f,m_choicePrompt,QColor(0,200,255,220),0.6f);
    for(int i=0;i<m_choiceOptions.size();++i){
        float y=0.3f-i*0.2f;
        QColor bg=m_choiceSelectedIndex==i?QColor(0,180,255,60):QColor(20,30,50,40);
        drawRectGL(-0.6f,y-0.07f,1.2f,0.15f,bg,true);
        QColor bc=m_choiceSelectedIndex==i?QColor(0,200,255,200):QColor(100,140,180,120);
        drawRectGL(-0.6f,y-0.07f,1.2f,0.15f,bc,false);
        drawTextGL(-0.55f,y,QString("%1. %2").arg(i+1).arg(m_choiceOptions[i]),QColor(200,220,255,200),0.5f);
    }
    drawTextGL(-0.3f,-0.5f,"↑↓ 选择 | ENTER 确认",QColor(150,180,200,180),0.5f);
}

void EchoMainWindow::showChoice(const QString& prompt, const QStringList& options) {
    m_choicePrompt=prompt; m_choiceOptions=options; m_choiceSelectedIndex=0;
    m_prevState=m_state; m_state=GameState::PlayerChoice;
}

// ═══════════════════════════════════════════
//  结局画面
// ═══════════════════════════════════════════

void EchoMainWindow::renderEndingScreen() {
    EndingType et=m_engine->calculateEnding();
    EndingInfo ei=m_engine->getEndingInfo(et);
    glClearColor(0.02f,0.02f,0.05f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
    for(int i=0;i<5;++i){float r=0.3f+i*0.2f+0.03f*sinf(m_gameTime+i),a=0.1f-i*0.02f;drawCircleGL(0,0.1f,r,QColor(0,150,255,int(a*255)),64);}
    drawTextGL(-0.3f,0.6f,"结局："+ei.title,QColor(0,200,255,220),1.2f);
    QString desc=ei.description;
    drawTextGL(-0.7f,0.2f,desc,QColor(180,200,220,180),0.45f);
    float b=0.6f+0.4f*sinf(m_gameTime*3.0f);
    drawTextGL(-0.3f,-0.4f,"按 ENTER 返回标题",QColor(0,200,255,int(b*200)),0.6f);
}

// ═══════════════════════════════════════════
//  视觉效果
// ═══════════════════════════════════════════

void EchoMainWindow::renderGlitchEffects() {
    float i=m_glitchIntensity;
    for(int j=0;j<5*i;++j){float y=(rand()%200-100)/100.0f,o=(rand()%20-10)/100.0f*i,a=i*(rand()%100)/100.0f;drawLineGL(-1.0f+o,y,1.0f+o,y,QColor(255,0,0,int(a*80)),1.0f);}
    for(auto& p:m_glitchParticles){float a=p.life/p.maxLife;drawRectGL(p.x,p.y,0.005f,0.005f,QColor(p.color.red(),p.color.green(),p.color.blue(),int(a*p.color.alpha())),true);}
    if(rand()%100<int(i*30)){float fa=i*0.08f;glClearColor(0.15f*fa,0.02f*fa,0.02f*fa,1.0f);}
}

void EchoMainWindow::renderParanoiaMeter() {
    float mx=0.88f,my=0.8f,mw=0.04f,mh=0.3f;
    drawRectGL(mx,my-mh,mw,mh,QColor(10,10,20,120),true); drawRectGL(mx,my-mh,mw,mh,QColor(60,80,120,40),false);
    float fh=mh*m_paranoiaDisplay;
    QColor fc=m_paranoiaDisplay<0.3f?QColor(0,180,255,150):(m_paranoiaDisplay<0.6f?QColor(255,200,0,150):QColor(255,40,40,180));
    drawRectGL(mx,my-fh,mw,fh,fc,true);
    drawTextGL(mx-0.02f,my+0.04f,"听觉偏执",QColor(150,180,200,150),0.25f);
    drawTextGL(mx-0.01f,my-mh-0.02f,QString("%1%").arg(int(m_paranoiaDisplay*100)),fc,0.25f);
}

void EchoMainWindow::renderAnxietyMeter() {
    float mx=0.88f,my=0.35f,mw=0.04f,mh=0.3f;
    drawRectGL(mx,my-mh,mw,mh,QColor(10,10,20,120),true); drawRectGL(mx,my-mh,mw,mh,QColor(120,60,60,40),false);
    float fh=mh*m_anxietyDisplay;
    QColor fc=m_anxietyDisplay<0.4f?QColor(0,200,150,120):(m_anxietyDisplay<0.7f?QColor(255,180,0,150):QColor(255,40,40,180));
    drawRectGL(mx,my-fh,mw,fh,fc,true);
    drawTextGL(mx-0.02f,my+0.04f,"焦虑值",QColor(200,150,150,150),0.25f);
}

void EchoMainWindow::renderSpectrumVisualization() {
    float by=-0.95f,mh=0.15f;
    for(size_t i=0;i<m_spectrumLines.size();++i){auto& l=m_spectrumLines[i];float x=-0.95f+i*1.9f/m_spectrumLines.size(),h=l.amplitude*mh*(0.5f+0.5f*m_paranoiaDisplay);QColor c=l.color;c.setAlpha(int(l.amplitude*80));drawRectGL(x,by,1.9f/m_spectrumLines.size()-0.001f,h,c,true);}
    if(m_paranoiaDisplay>0.5f) for(size_t i=0;i<m_spectrumLines.size()/8;++i){auto& l=m_spectrumLines[i];float x=-0.95f+i*1.9f/m_spectrumLines.size(),h=l.amplitude*mh*m_paranoiaDisplay;int a=int(m_paranoiaDisplay*l.amplitude*120);drawRectGL(x,by,1.9f/m_spectrumLines.size()-0.001f,h,QColor(255,40,40,a),true);}
}

void EchoMainWindow::renderHallucinations() {
    for(auto& h:m_hallucinations) drawTextGL(h.x,h.y,h.text,QColor(255,200,100,int(h.alpha*150)),0.5f);
}

void EchoMainWindow::renderNoiseDialogues() {
    for(auto& n:m_noiseTexts) drawTextGL(n.x,n.y,n.text,QColor(100,255,200,int(n.alpha*120)),0.45f);
}

void EchoMainWindow::renderSilenceOverlay() {
    float a=m_silenceAlpha;
    drawRectGL(-1.5f,-1.0f,3.0f,2.0f,QColor(0,0,0,int(a*200)),true);
    if(a>0.5f){float b=0.5f+0.3f*sinf(m_gameTime*2.0f);drawTextGL(-0.3f,0.0f,"静默中……",QColor(255,255,255,int(b*150)),0.8f);}
}

void EchoMainWindow::renderReverseIndicator() {
    if(m_engine->isReverseUnlocked()){
        float b=0.5f+0.3f*sinf(m_gameTime*2.0f);
        drawTextGL(-0.9f,-0.95f,"◄ 逆向播放已解锁",QColor(255,100,200,int(b*180)),0.35f);
    }
}

void EchoMainWindow::renderMorseIndicator() {
    if(m_engine->isMorseActive()){
        QString m=m_engine->getMorseMessage();
        if(!m.isEmpty()){float b=0.5f+0.4f*sinf(m_gameTime*5.0f);drawTextGL(0.5f,-0.95f,"[摩斯] "+m,QColor(255,200,100,int(b*150)),0.3f);}
    }
}

// ═══════════════════════════════════════════
//  绘制辅助
// ═══════════════════════════════════════════

void EchoMainWindow::drawTextGL(float x,float y,const QString& t,const QColor& c,float s){
    QPainter p(this); p.setPen(c);
    QFont f("Monospace",10); f.setStyleHint(QFont::Monospace); p.setFont(f);
    float asp=float(m_winWidth)/float(m_winHeight),ghw,ghh;
    if(asp>1.0f){ghw=asp;ghh=1.0f;}else{ghw=1.0f;ghh=1.0f/asp;}
    int sx=int((x+ghw)/(2.0f*ghw)*m_winWidth),sy=int((ghh-y)/(2.0f*ghh)*m_winHeight);
    p.save(); p.translate(sx,sy); p.scale(s,s); p.drawText(0,0,t); p.restore(); p.end();
}

void EchoMainWindow::drawRectGL(float x,float y,float w,float h,const QColor& c,bool f){
    glColor4f(c.redF(),c.greenF(),c.blueF(),c.alphaF());
    if(f){glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
    else{glBegin(GL_LINE_LOOP);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
}

void EchoMainWindow::drawLineGL(float x1,float y1,float x2,float y2,const QColor& c,float w){
    glColor4f(c.redF(),c.greenF(),c.blueF(),c.alphaF()); glLineWidth(w);
    glBegin(GL_LINES);glVertex2f(x1,y1);glVertex2f(x2,y2);glEnd(); glLineWidth(1.0f);
}

void EchoMainWindow::drawCircleGL(float cx,float cy,float r,const QColor& c,int seg){
    glColor4f(c.redF(),c.greenF(),c.blueF(),c.alphaF()); glBegin(GL_TRIANGLE_FAN);glVertex2f(cx,cy);
    for(int i=0;i<=seg;++i){float a=2.0f*3.1415926535f*i/seg;glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}glEnd();
}

QPointF EchoMainWindow::screenToGL(const QPoint& sp) const {
    float asp=float(m_winWidth)/float(m_winHeight),ghw,ghh;
    if(asp>1.0f){ghw=asp;ghh=1.0f;}else{ghw=1.0f;ghh=1.0f/asp;}
    return QPointF((sp.x()/float(m_winWidth))*2.0f*ghw-ghw,ghh-(sp.y()/float(m_winHeight))*2.0f*ghh);
}

int EchoMainWindow::hitTestFragment(const QPointF& gp) const {
    auto& frags=m_engine->fragments();
    if(m_state==GameState::FragmentSelect||m_state==GameState::Tutorial){
        float px=-0.9f,py=0.55f,pw=0.85f,ih=0.11f,gap=0.02f,so=m_fragmentScrollY;
        for(size_t i=0;i<frags.size();++i){auto& f=frags[i];if(f.isPlaced)continue;float y=py+(ih+gap)*float(i)+so;if(gp.x()>=px&&gp.x()<=px+pw&&gp.y()>=y-ih&&gp.y()<=y)return int(i);}
    }else{
        float pnx=-0.95f,pny=0.65f,iw=0.25f,ih=0.06f,gap=0.01f;int col=0,row=0,mc=3;
        for(size_t i=0;i<frags.size();++i){auto& f=frags[i];if(f.isPlaced){col++;if(col>=mc){col=0;row++;}continue;}float x=pnx+col*(iw+gap),y=pny-row*(ih+gap*3);if(gp.x()>=x&&gp.x()<=x+iw&&gp.y()>=y-ih&&gp.y()<=y)return int(i);col++;if(col>=mc){col=0;row++;}}
    }
    return -1;
}

int EchoMainWindow::hitTestTimelineSlot(const QPointF& gp) const {
    int sc=m_engine->timelineSlotCount();
    float sw=std::min(0.1f,(TIMELINE_W-(sc-1)*SLOT_GAP)/sc),tsw=sc*sw+(sc-1)*SLOT_GAP,ssx=TIMELINE_X+(TIMELINE_W-tsw)/2.0f;
    for(int i=0;i<sc;++i){float sx=ssx+i*(sw+SLOT_GAP),sy=TIMELINE_Y+0.02f,sh=TIMELINE_H-0.04f;if(gp.x()>=sx&&gp.x()<=sx+sw&&gp.y()>=sy&&gp.y()<=sy+sh)return i;}
    return -1;
}

// ═══════════════════════════════════════════
//  输入处理
// ═══════════════════════════════════════════

void EchoMainWindow::keyPressEvent(QKeyEvent* e) {
    switch(m_state){
    case GameState::Title:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){m_playerName.clear();m_state=GameState::NameInput;}
        break;
    case GameState::NameInput:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){
            m_engine->setPlayerName(m_playerName.trimmed());
            m_engine->generateFragments(GameChapter::Prologue);
            showChapterIntro();
        }else if(e->key()==Qt::Key_Escape){m_playerName.clear();m_state=GameState::Title;}
        else if(e->key()==Qt::Key_Backspace){if(!m_playerName.isEmpty())m_playerName.chop(1);}
        else if(!e->text().isEmpty()&&e->text().at(0).isPrint()){if(m_playerName.length()<12)m_playerName+=e->text();}
        break;
    case GameState::ChapterIntro:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){
            if(m_engine->currentChapter()==GameChapter::Prologue) m_state=GameState::Tutorial;
            else m_state=GameState::FragmentSelect;
            m_chapterIntroShown=true;
        }
        break;
    case GameState::Tutorial:
        if(e->key()==Qt::Key_Escape) m_state=GameState::Title;
        else if(e->key()==Qt::Key_Tab){m_state=GameState::TimelinePuzzle;m_selectedFragmentIndex=-1;}
        break;
    case GameState::FragmentSelect:
        if(e->key()==Qt::Key_Escape){m_state=GameState::Title;m_selectedFragmentIndex=-1;}
        else if(e->key()==Qt::Key_Tab){m_state=GameState::TimelinePuzzle;}
        else if(e->key()==Qt::Key_R&&m_selectedFragmentIndex>=0){
            auto& frags=m_engine->fragments();
            if(m_selectedFragmentIndex<int(frags.size())){
                m_engine->reverseFragment(frags[m_selectedFragmentIndex].id);
            }
        }
        break;
    case GameState::TimelinePuzzle:
        if(e->key()==Qt::Key_Escape){m_state=GameState::FragmentSelect;m_selectedFragmentIndex=-1;}
        else if(e->key()==Qt::Key_R){m_engine->resetCurrentChapter();m_selectedFragmentIndex=-1;m_glitchIntensity=0.0f;}
        else if(e->key()==Qt::Key_Tab&&m_selectedFragmentIndex>=0){
            auto& frags=m_engine->fragments();
            if(m_selectedFragmentIndex<int(frags.size())){
                m_engine->reverseFragment(frags[m_selectedFragmentIndex].id);
            }
        }
        break;
    case GameState::SceneComplete:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){
            GameChapter cur=m_engine->currentChapter();
            if(cur==GameChapter::Prologue){
                // 序章选择
                showChoice("你拼出来的对话中，林薇对陈远山说：'核心盘我藏好了，你没机会的。'\n陈远山冷笑：'你藏在一段声音里对吧？那我毁掉所有声音。'\n\n拼完后，耳机里传来一声不属于这段录音的叹息（老刘在门外敲了三下桌子）。\n你选择：",
                           {"忽略叹息，按标准流程提交报告","记录叹息，今晚加班重听所有录音"});
            }else if(cur==GameChapter::Chapter3){
                // 第三章关键选择
                showChoice("底噪意识从耳机里传出：'周宁，你别怕。你不是被夺舍的容器，你是我的选择。'\n你选择：",
                           {"接受自己是'林薇+周宁'的融合体","拒绝融合，用'纯粹周宁'身份继续（困难模式）","尝试彻底分离两人声音——逆向播放+高频阻断"});
            }else if(cur==GameChapter::Chapter4){
                showChoice("林薇的肉身就在隔离室里，极度虚弱。你面前只有一个操作：",
                           {"将林薇的完整人格声纹从你脑中转录回她体内","删除林薇肉体的生命维持系统录音，让密钥彻底消失"});
            }else{
                GameChapter next=static_cast<GameChapter>(static_cast<int>(cur)+1);
                if(next<=GameChapter::Finale){
                    m_currentMentorMessage=m_engine->getMentorMessage(static_cast<int>(cur));
                    m_mentorMessageIndex=static_cast<int>(cur);
                    m_mentorMessageAlpha=0.0f;
                    m_showMentorInput=(cur==GameChapter::Chapter3);
                    m_state=GameState::MentorMessage;
                }else{
                    m_state=GameState::EndingScreen;
                }
            }
        }else if(e->key()==Qt::Key_R){m_engine->resetCurrentChapter();m_selectedFragmentIndex=-1;m_glitchIntensity=0.0f;m_state=GameState::FragmentSelect;}
        break;
    case GameState::MentorMessage:
        if(m_showMentorInput){
            if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){m_showMentorInput=false;
                GameChapter next=static_cast<GameChapter>(static_cast<int>(m_engine->currentChapter())+1);
                if(next<=GameChapter::Finale){m_engine->generateFragments(next);showChapterIntro();}else{m_state=GameState::EndingScreen;}
                m_selectedFragmentIndex=-1;
            }else if(e->key()==Qt::Key_Backspace){if(!m_playerReply.isEmpty())m_playerReply.chop(1);}
            else if(!e->text().isEmpty()&&e->text().at(0).isPrint())m_playerReply+=e->text();
        }else{if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){
            GameChapter next=static_cast<GameChapter>(static_cast<int>(m_engine->currentChapter())+1);
            if(next<=GameChapter::Finale){m_engine->generateFragments(next);showChapterIntro();}else{m_state=GameState::EndingScreen;}
            m_selectedFragmentIndex=-1;
        }}break;
    case GameState::PlayerChoice:
        if(e->key()==Qt::Key_Up||e->key()==Qt::Key_W){if(m_choiceSelectedIndex>0)m_choiceSelectedIndex--;}
        else if(e->key()==Qt::Key_Down||e->key()==Qt::Key_S){if(m_choiceSelectedIndex<m_choiceOptions.size()-1)m_choiceSelectedIndex++;}
        else if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){
            PlayerChoice pc=PlayerChoice::None;
            GameChapter cur=m_engine->currentChapter();
            if(cur==GameChapter::Prologue){
                pc=(m_choiceSelectedIndex==0)?PlayerChoice::IgnoreSigh:PlayerChoice::RecordSigh;
                m_engine->recordChoice(pc);
                m_engine->generateFragments(GameChapter::Chapter1);
                showChapterIntro();
            }else if(cur==GameChapter::Chapter3){
                if(m_choiceSelectedIndex==0) pc=PlayerChoice::AcceptFusion;
                else if(m_choiceSelectedIndex==1) pc=PlayerChoice::RejectFusion;
                else pc=PlayerChoice::SeparateVoices;
                m_engine->recordChoice(pc);
                m_engine->generateFragments(GameChapter::Chapter4);
                showChapterIntro();
            }else if(cur==GameChapter::Chapter4){
                pc=(m_choiceSelectedIndex==0)?PlayerChoice::TransferBack:PlayerChoice::DeleteLifeSupport;
                m_engine->recordChoice(pc);
                m_state=GameState::EndingScreen;
            }
        }else if(e->key()==Qt::Key_Escape){m_state=m_prevState;}
        break;
    case GameState::EndingScreen:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter) m_state=GameState::GameOver;
        break;
    case GameState::GameOver:
        if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter){m_engine->generateFragments(GameChapter::Prologue);showChapterIntro();m_selectedFragmentIndex=-1;m_mentorMessageIndex=0;m_playerReply.clear();}
        break;
    }
    update();
}

void EchoMainWindow::mousePressEvent(QMouseEvent* e) {
    QPointF gp=screenToGL(e->pos());
    if(m_state==GameState::FragmentSelect||m_state==GameState::TimelinePuzzle||m_state==GameState::Tutorial){
        if(e->button()==Qt::LeftButton){int fi=hitTestFragment(gp);if(fi>=0){m_selectedFragmentIndex=fi;m_isDragging=true;m_dragStartPos=e->pos();m_dragCurrentPos=e->pos();if(m_state==GameState::FragmentSelect||m_state==GameState::Tutorial)m_state=GameState::TimelinePuzzle;}}
        else if(e->button()==Qt::RightButton){m_selectedFragmentIndex=-1;m_isDragging=false;}
    }
}

void EchoMainWindow::mouseMoveEvent(QMouseEvent* e) { if(m_isDragging) m_dragCurrentPos=e->pos(); }

void EchoMainWindow::mouseReleaseEvent(QMouseEvent* e) {
    if(m_isDragging&&e->button()==Qt::LeftButton){m_isDragging=false;QPointF gp=screenToGL(e->pos());
        if(m_state==GameState::TimelinePuzzle&&m_selectedFragmentIndex>=0){int si=hitTestTimelineSlot(gp);
            if(si>=0){auto& frags=m_engine->fragments();if(m_selectedFragmentIndex<int(frags.size())){m_engine->placeFragment(frags[m_selectedFragmentIndex].id,si);}}m_selectedFragmentIndex=-1;}}
    if(e->button()==Qt::RightButton&&m_state==GameState::TimelinePuzzle){QPointF gp=screenToGL(e->pos());int si=hitTestTimelineSlot(gp);if(si>=0)m_engine->removeFragment(si);}
}

// ═══════════════════════════════════════════
//  信号槽
// ═══════════════════════════════════════════

void EchoMainWindow::onFragmentPlaced(int,int){}
void EchoMainWindow::onFragmentRemoved(int,int){}
void EchoMainWindow::onSceneCompleted(int ch,bool d){
    m_state=GameState::SceneComplete;
    // chapter index 映射到 mentor message index（0=Prologue→msg0, 1=Ch1→msg1, ...）
    // 最多 9 条消息，做边界保护
    m_mentorMessageIndex = std::min(ch, 8);
    if(d) m_glitchIntensity=1.0f;
}
void EchoMainWindow::onParanoiaChanged(float){}
void EchoMainWindow::onAnxietyChanged(float){}
void EchoMainWindow::onReverseUnlocked(){}
void EchoMainWindow::onHallucinationTriggered(const QString& m){
    HallucinationText ht;ht.text=m;ht.x=(rand()%160-80)/100.0f;ht.y=(rand()%120-60)/100.0f;ht.alpha=0.8f;ht.speedX=(rand()%20-10)/200.0f;ht.speedY=(rand()%20-10)/200.0f;ht.life=4.0f+(rand()%30)/10.0f;m_hallucinations.push_back(ht);
}
void EchoMainWindow::onNoiseCommunion(const QString& m){
    FloatingNoise fn;fn.text=m;fn.x=(rand()%100-50)/100.0f;fn.y=-0.3f+(rand()%60)/100.0f;fn.alpha=0.7f;fn.life=3.0f+(rand()%20)/10.0f;m_noiseTexts.push_back(fn);
}
