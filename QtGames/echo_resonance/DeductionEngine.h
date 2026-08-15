#pragma once
#include "DeductionData.h"
#include <QObject>
#include <QMap>

// ═══════════════════════════════════════════════════════════════
//  声纹推演引擎
//  管理命题、推演合成、三大心理陷阱（回声强化/负片覆盖/自主学习）
// ═══════════════════════════════════════════════════════════════
class DeductionEngine : public QObject {
    Q_OBJECT
public:
    explicit DeductionEngine(QObject* parent = nullptr);

    // ── 样本库 ──
    const std::vector<DeductionSample>& samples() const { return m_samples; }
    DeductionSample* findSample(int id);
    void lockSample(int id);              // 锁存（防负片覆盖）
    bool isSampleLocked(int id) const;

    // ── 命题 ──
    const std::vector<Proposition>& propositions() const { return m_propositions; }
    Proposition* currentProposition();     // 当前待验证命题
    void resolveProposition(int id);       // 标记命题已推演

    // ── 推演核心 ──
    // 放入两个以上样本，设定时间锚点相对关系，生成推演结果
    DeductionResult deduce(const std::vector<DeductionSlot>& boardSlots,
        const std::vector<Relation>& relations);

    // ── 心理变量 ──
    PsychProfile& profile() { return m_profile; }
    void setGreedFear(float greed, float fear);
    void recordAnchor(TimeAnchor a);       // 记录偏好的时间锚点
    void recordEmotion(bool fearPreferred); // 记录情绪偏好（恐惧 vs 怀旧）

    // ── 三大心理陷阱 ──
    // 1. 回声强化：连续推演某人"有罪"3次 → 其样本音高升高0.2八度
    void applyEchoReinforcement(int sampleId, bool convicted);
    // 2. 负片记忆覆盖：未锁存样本被推演结果覆盖
    QStringList negativeOverwrite(int sampleId, const QString& newMemory);
    // 3. 自主学习：生成玩家人格侧写
    PersonalityProfile generatePersonalityProfile() const;

    // ── 终章三槽推演 ──
    QString generateFinalAudioDescription(const std::vector<DeductionSlot>& boardSlots);

    // ── 剧情数据（章节命题序列）──
    void setupPrologue();   // 序章：例行推演
    void setupChapter1();   // 第一章：推演他人
    void setupChapter2();   // 第二章：推演自我
    void setupChapter3();   // 第三章：推演关系
    void setupFinale();     // 终章：推演未来

    // 获取当前章节
    int chapter() const { return m_chapter; }
    void setChapter(int c) { m_chapter = c; }

signals:
    void propositionResolved(int id);
    void memoryRewritten(const QString& description);
    void echoReinforced(int sampleId);

private:
    std::vector<DeductionSample> m_samples;
    std::vector<Proposition> m_propositions;
    PsychProfile m_profile;
    int m_chapter = 0;
    int m_nextSampleId = 0;
    int m_nextPropositionId = 0;
    QMap<int, int> m_convictCount;  // 样本被定罪次数（回声强化）
    QMap<int, QString> m_overwrittenMemories; // 被负片覆盖的记忆

    DeductionSample addSample(const QString& name, const QString& desc,
        SampleKind kind, const QString& source);
    Proposition addProposition(const QString& text, const QStringList& hints);
};
