#pragma once
#include <QString>
#include <QStringList>
#include <QColor>
#include <vector>
#include "CommonFragment.h"  // 复用公共常量（ECHO_PI、DEFAULT_PLAYER_NAME 等）

// ═══════════════════════════════════════════════════════════════
//  声纹推演盘 —— 核心数据结构
//  推演取代回溯：由玩家的因果推断主动触发剧情
// ═══════════════════════════════════════════════════════════════

// ── 三要素之一：声纹样本（人物/环境/情感印记）──
enum class SampleKind {
    Character,   // 人物声纹
    Environment, // 环境声
    Emotion      // 情感印记
};

// ── 声灵类型（离奇魔幻基因，需在 DeductionSample 之前定义）──
enum class SpiritKind {
    PrimeEcho,    // 原初嗡鸣（源头）
    Mirror,       // 镜声灵（反射）
    Silence,      // 默声灵（消除）
    Lie,          // 谎声灵（伪造）
    Shattered,    // 碎璃声灵（背叛）
    Wrath,        // 愤怒声灵
    Count
};

struct DeductionSample {
    int id = -1;
    QString name;          // 样本名
    QString description;   // 描述
    SampleKind kind = SampleKind::Character;
    QString source;        // 来源人物/地点
    float basePitch = 1.0f; // 基础音高（1.0 = 原始）
    bool locked = false;    // 是否已锁存（防止负片记忆覆盖）
    int suspicionWeight = 0; // 被推演为"有罪"的累计次数（回声强化）
    SpiritKind spiritKind = SpiritKind::PrimeEcho; // 关联的声灵（离奇魔幻基因）
    QColor color() const {
        switch (kind) {
        case SampleKind::Character:   return QColor(0,180,255);
        case SampleKind::Environment: return QColor(0,220,160);
        case SampleKind::Emotion:     return QColor(255,150,80);
        }
        return Qt::white;
    }
    QString kindLabel() const {
        switch (kind) {
        case SampleKind::Character:   return "人物";
        case SampleKind::Environment: return "环境";
        case SampleKind::Emotion:     return "情感";
        }
        return "";
    }
};

// ── 三要素之二：干预动作 ──
enum class Intervention {
    Play,     // 播放
    Trim,     // 删减
    Pitch,    // 变调
    Forge,    // 伪造
    Count
};

// ── 三要素之三：时间锚点 ──
enum class TimeAnchor {
    Past,      // 过去
    Present,   // 现在
    Future,    // 未来
    Count
};

// ── 推演盘三槽 ──
struct DeductionSlot {
    int sampleId = -1;         // 声纹样本 ID
    Intervention action = Intervention::Play;  // 干预动作
    TimeAnchor anchor = TimeAnchor::Past;      // 时间锚点
};

// ── 相对关系（掩盖/激发/反转/同步）──
enum class Relation {
    Mask,      // 掩盖
    Excite,    // 激发
    Reverse,   // 反转
    Sync,      // 同步
    Count
};

// ── 待验证命题 ──
struct Proposition {
    int id = -1;
    QString text;              // 命题文本
    QStringList hints;         // 提示
    bool resolved = false;     // 是否已推演
    // 心理可信度（0.0 = 纯粹偏见回声，1.0 = 真相的影子）
    float psychologicalCredibility = 0.0f;
};

// ── 推演结果 ──
struct DeductionResult {
    QString audioDescription;  // 推演音频的文字描述（视觉降级时用）
    QString logicInference;    // 逻辑推断结论
    float credibility = 0.5f;  // 心理可信度
    bool rewritesMemory = false; // 是否改写记忆库
    QStringList memoryEffects; // 记忆改写效果描述
};

// ── 心理变量（贪婪/恐惧比例等）──
struct PsychProfile {
    float greed = 0.5f;        // 贪婪 0.0-1.0
    float fear = 0.5f;         // 恐惧 0.0-1.0
    // 偏好的时间锚点统计
    int pastCount = 0;
    int presentCount = 0;
    int futureCount = 0;
    // 偏好的情绪变量
    int fearEmotionCount = 0;
    int nostalgiaCount = 0;
    // 放过/定罪统计
    int spareCount = 0;
    int convictCount = 0;
};

// ── 推演盘自主学习的人格侧写 ──
struct PersonalityProfile {
    QString preferAnchor;   // 偏好时间锚点
    QString preferEmotion;  // 偏好情绪
    QString verdict;        // 对陈远山的最终判断
    int totalDeductions = 0;
    QString summary;        // 心理侧写报告文本
};

// ═══════════════════════════════════════════════════════════════
//  离奇魔幻基因：声灵系统
//  声音从物理现象升维为"有灵性、可诅咒、能契约"的古老存在
// ═══════════════════════════════════════════════════════════════

// ── 声灵结构（有自我意识、立场、好感度）──
struct SpiritInfo {
    SpiritKind kind = SpiritKind::PrimeEcho;
    QString name;          // 声灵名
    QString title;         // 称号
    QString stance;        // 立场/欲望
    float affinity = 0.0f; // 好感度 0.0-1.0（越召唤越高）
    bool manifested = false; // 是否已显形（光晕）
    QColor color() const {
        switch (kind) {
        case SpiritKind::PrimeEcho:  return QColor(255, 220, 120); // 金色
        case SpiritKind::Mirror:     return QColor(150, 200, 255); // 淡蓝
        case SpiritKind::Silence:    return QColor(200, 180, 255); // 淡紫
        case SpiritKind::Lie:        return QColor(255, 140, 180); // 粉红
        case SpiritKind::Shattered:  return QColor(120, 230, 180); // 青绿
        case SpiritKind::Wrath:      return QColor(255, 90, 90);   // 血红
        case SpiritKind::Count:      return Qt::white;
        }
        return Qt::white;
    }
};

// ── 灵魂碎片（割舍声音后收集，拼成林薇母亲的脸）──
struct SoulFragment {
    int id = -1;
    QString name;      // 碎片名
    QString source;    // 来源（割舍的声音/释放的声灵）
    bool collected = false;
    bool glowing = false; // 泛光状态
};

// ── 契约（与声灵缔结）──
enum class ContractSpirit {
    None,
    Mirror,   // 镜声灵契约：说出最恐惧的真话
    Silence,  // 默声灵契约：删除最喜欢的记忆
    Lie,      // 谎声灵契约：撒一个会被识破的谎
    Count
};

// ── 偏差能量（原初嗡鸣倒计时）──
struct DeviationEnergy {
    float amount = 0.0f;   // 偏差能量 0.0-1.0
    bool overloaded = false; // 是否触发嗡鸣
    int releaseCount = 0;    // 释放声灵次数（触发逆向推演）
};
