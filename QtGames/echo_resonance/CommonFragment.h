#pragma once
#include <QString>
#include <QColor>
#include <vector>

// 圆周率（float），统一宏，避免各处硬编码 3.1415926535f
#ifndef ECHO_PI
#define ECHO_PI 3.141592653589793f
#endif

// 玩家角色名的统一默认值与占位符
// 所有叙事文案中用 {name} 占位，运行时统一替换为玩家输入的名字（空则用此默认值）
inline const QString DEFAULT_PLAYER_NAME = QStringLiteral("小周");
inline const QString PLAYER_NAME_PLACEHOLDER = QStringLiteral("{name}");

// ── 声音碎片类型 ──
enum class FragmentType {
    Footstep,      ElectricBuzz,  WaterDrop,     Heartbeat,
    VoiceWhisper,  StaticNoise,   DoorCreak,     Typewriter,
    Breath,        LowFrequency,  MetalExpansion, // 金属膨胀声
    Geomagnetic,   // 地磁波动
    ClockTick,     // 时钟滴答
    TireScreech,   // 轮胎摩擦
    RainAmbient,   // 雨声
    Count
};

// ── 可信度等级 ──
enum class Credibility {
    Real = 0,      // 真实残留 - 荧光蓝
    Suspicious,    // 可疑 - 青色
    Noise,         // 设备噪声 - 黄色
    Fake           // 伪造声纹 - 红色
};

// ── 锚点音类型 ──
enum class AnchorType {
    None,
    DoorBeep,      // 门禁刷卡声
    CoffeeMachine, // 咖啡机启动声
    HeartMonitor,  // 心率监护仪
    ClockChime,    // 钟声
    KeyTurn,       // 钥匙转动
};

// ── 游戏章节 ──
enum class GameChapter {
    Prologue = 0,   // 序章：入职第7天
    Chapter1 = 1,   // 日常的裂隙
    Chapter2 = 2,   // 地下三十米
    Chapter3 = 3,   // 镜像之家
    Chapter4 = 4,   // 声牢
    Finale   = 5,   // 终章
};

// ── 游戏结局 ──
enum class EndingType {
    None = -1,
    SilentArchive,  // 静默档案
    EchoOrphan,     // 回声孤儿
    Duet,           // 双声部
    Matricide,      // 弑母
    Rewinder,       // 倒带者（隐藏）
    ZeroDecibel,    // 零分贝（真结局）
    Count
};

// ── 玩家选择枚举 ──
enum class PlayerChoice {
    // 序章选择
    IgnoreSigh,       // 忽略叹息
    RecordSigh,       // 记录叹息，加班重听
    // 第三章选择
    AcceptFusion,     // 接受融合
    RejectFusion,     // 拒绝融合（困难模式）
    SeparateVoices,   // 尝试分离
    // 第四章选择
    TransferBack,     // 转录回林薇
    DeleteLifeSupport,// 删除生命维持
    None,
};

// ── 单个声音碎片 ──
struct CommonFragment {
    FragmentType type;
    QString name;
    QString description;
    Credibility credibility;
    float durationSec;
    float frequencyHz;
    float amplitude;
    bool isPlaced = false;
    int timelineSlot = -1;
    int id;
    std::vector<float> spectrumSamples;
    bool isAnchor = false;        // 是否为锚点音
    AnchorType anchorType = AnchorType::None;
    bool isReversed = false;      // 是否已逆向播放
    bool revealsSecret = false;   // 逆向播放时揭示秘密
    QString reverseDescription;   // 逆向播放时的描述

    QColor displayColor() const {
        switch (credibility) {
            case Credibility::Real:       return QColor(0, 180, 255);
            case Credibility::Suspicious: return QColor(0, 220, 200);
            case Credibility::Noise:      return QColor(255, 200, 0);
            case Credibility::Fake:       return QColor(255, 40, 40);
        }
        return Qt::white;
    }

    QString credibilityLabel() const {
        switch (credibility) {
            case Credibility::Real:       return "真实残留";
            case Credibility::Suspicious: return "可疑信号";
            case Credibility::Noise:      return "设备噪声";
            case Credibility::Fake:       return "伪造声纹";
        }
        return "";
    }
};

// ── 时间轴槽位 ──
struct TimelineSlot {
    int index;
    int fragmentId = -1;
    bool isLocked = false;
    bool isAnchorSlot = false;     // 是否为锚点槽位
    AnchorType requiredAnchor = AnchorType::None;
};

// ── 音频场景 ──
struct AudioScene {
    QString name;
    QString description;
    int sceneId;
    bool isDistorted = false;
    float coherenceScore = 0.0f;
};

// ── 角色信息 ──
struct CharacterInfo {
    QString name;
    QString title;
    QString secret;
    bool isRevealed = false;
};

// ── 结局信息 ──
struct EndingInfo {
    EndingType type;
    QString title;
    QString description;
    QString condition;  // 达成条件描述
    bool unlocked = false;
};

// ── 选择记录 ──
struct ChoiceRecord {
    PlayerChoice choice;
    GameChapter chapter;
    QString description;
};

// ── 回声碎片（支线叙事）──
struct EchoFragment {
    QString title;         // 碎片名
    QString location;      // 触发地点
    QString content;       // 内容
    QString reward;        // 解锁奖励
    bool unlocked = false; // 是否已解锁
};

// ── 结局余波录音 ──
struct EpilogueRecord {
    EndingType ending;
    QString content;       // 余波录音文本（约90秒旁白）
};

// ── 被遗忘者名单条目 ──
struct ForgottenEntry {
    QString name;
    QString epitaph;       // 墓志铭/一句话
};

// ── 何悦分支状态 ──
enum class HeYueState {
    Unknown,      // 未触发
    Hostage,      // 被囚禁，待抉择
    Rescued,      // 成功救出
    Brainwashed,  // 被洗脑成傀儡
};

// ── 碎片回声（衰减回声，呼应"回声迷宫"）──
struct FragmentEcho {
    int sourceFragmentId;  // 源碎片
    float remainingTime;   // 剩余持续时间（秒）
    float maxTime;         // 最大持续时间
    float intensity;       // 强度（随放置次数共振增强）
};

// ── 老刘静默摩斯（沉默中浮现的声音）──
struct SilenceMorse {
    QString message;    // 摩斯信息
    float threshold;    // 需要的静默时长
    bool revealed = false;
};

// ── 选择点类型 ──
enum class ChoicePoint {
    PrologueSigh,   // 序章：老刘的叹息
    HeYueHostage,   // 第三章：何悦被囚禁
    Identity,       // 第三章：自我声纹认同
    LifeTransfer,   // 第四章：林薇生死
};

// ── 动态文案模板 ──
// 用 {player} 占位玩家名，用 {suffix} 占位根据状态动态拼接的附加段落
struct ChoiceTemplate {
    ChoicePoint point;
    QString promptBase;      // 提示基础文本（含 {player} 占位符）
    QStringList options;     // 选项（含 {player} 占位符）
};
