#include "DeductionEngine.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

DeductionEngine::DeductionEngine(QObject* parent) : QObject(parent) {}

// ═══════════════════════════════════════════════════════════════
//  样本库构建
// ═══════════════════════════════════════════════════════════════

DeductionSample DeductionEngine::addSample(const QString& name, const QString& desc,
    SampleKind kind, const QString& source)
{
    DeductionSample s;
    s.id = m_nextSampleId++;
    s.name = name;
    s.description = desc;
    s.kind = kind;
    s.source = source;
    m_samples.push_back(s);
    return s;
}

Proposition DeductionEngine::addProposition(const QString& text, const QStringList& hints)
{
    Proposition p;
    p.id = m_nextPropositionId++;
    p.text = text;
    p.hints = hints;
    m_propositions.push_back(p);
    return p;
}

// ═══════════════════════════════════════════════════════════════
//  序章：例行推演
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setupPrologue()
{
    m_samples.clear();
    m_propositions.clear();
    m_chapter = 0;

    addSample("林薇的日常语调", "平淡、清晰，无情绪波动", SampleKind::Character, "林薇");
    addSample("林薇与陈远山最后一次对话", "背景中有异常的玻璃摩擦声", SampleKind::Character, "林薇/陈远山");
    addSample("林薇的书面便签", "「别推演我，推演你自己。」", SampleKind::Emotion, "林薇");

    addProposition("林薇在失踪前72小时的心理状态：她是否蓄谋叛逃？",
        { "玻璃摩擦声放在'过去'是偶然杂音，放在'未来'预示暴力" });
}

// ═══════════════════════════════════════════════════════════════
//  第一章：推演他人
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setupChapter1()
{
    m_samples.clear();
    m_propositions.clear();
    m_chapter = 1;

    addSample("老刘的沉默呼吸", "规律、低沉，有轻微的停顿", SampleKind::Character, "老刘");
    addSample("门卫室铁门开合声", "沉重、缓慢，铰链有锈蚀声", SampleKind::Environment, "门卫室");
    addSample("何悦哼唱旋律", "《小星星》，干净无杂质", SampleKind::Character, "何悦");
    addSample("空调低频噪音", "持续的 50Hz 嗡鸣", SampleKind::Environment, "实验室空调");
    addSample("陈远山单侧听力测试报告", "右耳波形平坦，左耳正常", SampleKind::Character, "陈远山");
    addSample("陈远山关门声", "左侧总比右侧轻1分贝", SampleKind::Environment, "陈远山办公室");

    addProposition("林薇把核心算法藏在了哪里？",
        { "老刘是否知道核心盘位置？", "何悦哼的歌里是否含有情报？", "陈远山的右耳失聪是否影响决策？" });
}

// ═══════════════════════════════════════════════════════════════
//  第二章：推演自我
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setupChapter2()
{
    m_samples.clear();
    m_propositions.clear();
    m_chapter = 2;

    addSample("刹车声", "尖锐刺耳的轮胎摩擦", SampleKind::Environment, "车祸现场");
    addSample("金属扭曲", "车身变形的沉闷巨响", SampleKind::Environment, "车祸现场");
    addSample("林薇的哭泣", "压抑、颤抖的抽泣", SampleKind::Emotion, "林薇");
    addSample("心电图停止音", "持续的'滴——'长鸣", SampleKind::Environment, "医院");
    addSample("手术器械声", "金属碰撞的清脆声", SampleKind::Environment, "手术室");
    addSample("林薇的指令声", "急促而专业", SampleKind::Character, "林薇");

    addProposition("我（周宁）是真实的人，还是林薇声纹的容器？",
        { "心电图停止音与林薇哭泣的先后顺序决定她是救助者还是诱因", "仪器滴声的变速暗示移植是否仓促" });
}

// ═══════════════════════════════════════════════════════════════
//  第三章：推演关系（博弈模拟）
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setupChapter3()
{
    m_samples.clear();
    m_propositions.clear();
    m_chapter = 3;

    addSample("陈远山的愤怒爆发", "声压极高，集中在左声道", SampleKind::Character, "陈远山");
    addSample("陈远山的平静陈述", "尾音微颤，呼吸不稳", SampleKind::Character, "陈远山");
    addSample("紧急按钮声", "机械按下的咔哒声", SampleKind::Environment, "实验室");
    addSample("林薇的挑衅语音（伪造）", "你在推演中伪造的样本", SampleKind::Emotion, "推演盘生成");

    addProposition("如果在最终对峙中，我对陈远山说X，他会怎么做？",
        { "语言回应", "声学回应", "行为回应" });
}

// ═══════════════════════════════════════════════════════════════
//  终章：推演未来
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setupFinale()
{
    m_samples.clear();
    m_propositions.clear();
    m_chapter = 4;

    // 三个空槽可放入的样本（牺牲/传递/定义）
    addSample("林薇的生命维持低频", "0.5Hz，微弱的生命信号", SampleKind::Emotion, "林薇");
    addSample("何悦的清洁声纹", "天然干净，无信息污染", SampleKind::Character, "何悦");
    addSample("你自己的日常呼吸声", "平静而规律", SampleKind::Character, "周宁");
    addSample("老刘的摩斯密码", "7下敲击，代表'GO'", SampleKind::Character, "老刘");
    addSample("走廊掌声", "1989年A-Lab成立典礼的掌声", SampleKind::Environment, "走廊");
    addSample("男孩日记的数字幻觉", "第114514只羊", SampleKind::Emotion, "第7号实验体");

    addProposition("我推演出的最佳结局，需要用哪种代价交换？",
        { "第一槽：牺牲", "第二槽：传递", "第三槽：定义" });
}

// ═══════════════════════════════════════════════════════════════
//  样本查找 / 锁存
// ═══════════════════════════════════════════════════════════════

DeductionSample* DeductionEngine::findSample(int id)
{
    for (auto& s : m_samples) if (s.id == id) return &s;
    return nullptr;
}

void DeductionEngine::lockSample(int id)
{
    auto* s = findSample(id);
    if (s) s->locked = true;
}

bool DeductionEngine::isSampleLocked(int id) const
{
    for (const auto& s : m_samples) if (s.id == id) return s.locked;
    return false;
}

// ═══════════════════════════════════════════════════════════════
//  命题
// ═══════════════════════════════════════════════════════════════

Proposition* DeductionEngine::currentProposition()
{
    for (auto& p : m_propositions) if (!p.resolved) return &p;
    if (!m_propositions.empty()) return &m_propositions.back();
    return nullptr;
}

void DeductionEngine::resolveProposition(int id)
{
    for (auto& p : m_propositions) if (p.id == id) { p.resolved = true; break; }
    emit propositionResolved(id);
}

// ═══════════════════════════════════════════════════════════════
//  推演核心：由样本+动作+锚点+关系合成推演结果
// ═══════════════════════════════════════════════════════════════

DeductionResult DeductionEngine::deduce(const std::vector<DeductionSlot>& boardSlots,
    const std::vector<Relation>& relations)
{
    DeductionResult r;
    if (boardSlots.size() < 2) {
        r.audioDescription = "需要至少两个声纹样本才能推演。";
        r.logicInference = "推演板拒绝空转。";
        r.credibility = 0.0f;
        return r;
    }

    // 组装推演音频描述（程序实时合成的文字描述，实际音频由 DeductionAudio 变调叠加）
    QStringList desc;
    for (const auto& slot : boardSlots) {
        auto* s = findSample(slot.sampleId);
        if (!s) continue;
        QString action;
        switch (slot.action) {
        case Intervention::Play:  action = "播放"; break;
        case Intervention::Trim:  action = "删减"; break;
        case Intervention::Pitch: action = "变调"; break;
        case Intervention::Forge: action = "伪造"; break;
        default: action = "播放"; break;
        }
        QString anchor;
        switch (slot.anchor) {
        case TimeAnchor::Past:    anchor = "过去"; break;
        case TimeAnchor::Present: anchor = "现在"; break;
        case TimeAnchor::Future:  anchor = "未来"; break;
        default: anchor = "过去"; break;
        }
        desc << QString("%1[%2] 于%3").arg(s->name).arg(action).arg(anchor);
    }

    // 关系描述
    QStringList relDesc;
    for (const auto& rel : relations) {
        switch (rel) {
        case Relation::Mask:    relDesc << "掩盖"; break;
        case Relation::Excite:  relDesc << "激发"; break;
        case Relation::Reverse: relDesc << "反转"; break;
        case Relation::Sync:    relDesc << "同步"; break;
        default: break;
        }
    }

    r.audioDescription = "推演音频：" + desc.join(" + ");
    if (!relDesc.isEmpty()) r.audioDescription += "（关系：" + relDesc.join("/") + "）";

    // 心理可信度：受样本多样性、关系明确度、心理变量影响
    float baseCred = 0.4f + 0.1f * static_cast<float>(boardSlots.size());
    if (boardSlots.size() >= 3) baseCred += 0.1f;
    if (relations.size() >= 2) baseCred += 0.1f;
    // 贪婪/恐惧比例越极端，越可能是偏见回声
    float bias = fabsf(m_profile.greed - m_profile.fear);
    baseCred -= bias * 0.2f;
    r.credibility = std::max(0.0f, std::min(1.0f, baseCred));

    // 逻辑推断
    bool hasFutureAnchor = false;
    for (const auto& s : boardSlots) if (s.anchor == TimeAnchor::Future) hasFutureAnchor = true;
    if (hasFutureAnchor) {
        r.logicInference = "推演显示：这段声纹关系指向未来——你在用未来解释过去。";
    } else {
        r.logicInference = "推演显示：所有样本锚定在过去，这是一次对既成事实的重构。";
    }

    // 记忆改写判定
    r.rewritesMemory = (r.credibility > 0.65f);
    if (r.rewritesMemory) {
        r.memoryEffects << "推演结果正在改写你的记忆库。";
    }
    return r;
}

// ═══════════════════════════════════════════════════════════════
//  心理变量
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::setGreedFear(float greed, float fear)
{
    m_profile.greed = std::max(0.0f, std::min(1.0f, greed));
    m_profile.fear = std::max(0.0f, std::min(1.0f, fear));
}

void DeductionEngine::recordAnchor(TimeAnchor a)
{
    switch (a) {
    case TimeAnchor::Past:    m_profile.pastCount++; break;
    case TimeAnchor::Present: m_profile.presentCount++; break;
    case TimeAnchor::Future:  m_profile.futureCount++; break;
    default: break;
    }
}

void DeductionEngine::recordEmotion(bool fearPreferred)
{
    if (fearPreferred) m_profile.fearEmotionCount++;
    else m_profile.nostalgiaCount++;
}

// ═══════════════════════════════════════════════════════════════
//  心理陷阱 1：回声强化
// ═══════════════════════════════════════════════════════════════

void DeductionEngine::applyEchoReinforcement(int sampleId, bool convicted)
{
    if (convicted) {
        m_convictCount[sampleId]++;
    } else {
        m_convictCount[sampleId] = std::max(0, m_convictCount[sampleId] - 1);
    }
    auto* s = findSample(sampleId);
    if (!s) return;
    if (m_convictCount[sampleId] >= 3) {
        // 连续3次"有罪"→ 音高升高0.2八度（听起来更刺耳）
        s->basePitch = 1.0f + 0.2f * std::pow(2.0f, (m_convictCount[sampleId] - 3) / 12.0f);
        emit echoReinforced(sampleId);
    } else {
        s->basePitch = 1.0f;
    }
}

// ═══════════════════════════════════════════════════════════════
//  心理陷阱 2：负片记忆覆盖
// ═══════════════════════════════════════════════════════════════

QStringList DeductionEngine::negativeOverwrite(int sampleId, const QString& newMemory)
{
    QStringList effects;
    auto* s = findSample(sampleId);
    if (!s) return effects;
    if (s->locked) {
        effects << QString("样本「%1」已锁存，未被覆盖。").arg(s->name);
        return effects;
    }
    m_overwrittenMemories[sampleId] = newMemory;
    s->description = newMemory;
    effects << QString("样本「%1」的记忆被推演结果覆盖：%2").arg(s->name).arg(newMemory);
    emit memoryRewritten(effects.last());
    return effects;
}

// ═══════════════════════════════════════════════════════════════
//  心理陷阱 3：自主学习（人格侧写）
// ═══════════════════════════════════════════════════════════════

PersonalityProfile DeductionEngine::generatePersonalityProfile() const
{
    PersonalityProfile pp;
    pp.totalDeductions = m_profile.pastCount + m_profile.presentCount + m_profile.futureCount;

    // 偏好时间锚点
    int maxA = std::max({ m_profile.pastCount, m_profile.presentCount, m_profile.futureCount });
    if (maxA == m_profile.futureCount && maxA > 0) pp.preferAnchor = "未来";
    else if (maxA == m_profile.presentCount && maxA > 0) pp.preferAnchor = "现在";
    else pp.preferAnchor = "过去";

    // 偏好情绪
    if (m_profile.fearEmotionCount >= m_profile.nostalgiaCount) pp.preferEmotion = "恐惧";
    else pp.preferEmotion = "怀旧";

    // 对陈远山的判断
    if (m_profile.greed > m_profile.fear) pp.verdict = "治愈他的耳朵";
    else pp.verdict = "永久静音他的世界";

    pp.summary = QString(
        "你是一个偏好用「%1」%2「%3」的推演者。"
        "你在第%4次推演中完成了最后一次因果重构。"
        "推演板不会撒谎——它只是回声。")
        .arg(pp.preferAnchor)
        .arg(pp.preferAnchor == "未来" ? "逃避" : "审视")
        .arg(pp.preferEmotion)
        .arg(pp.totalDeductions);
    return pp;
}

// ═══════════════════════════════════════════════════════════════
//  终章三槽推演
// ═══════════════════════════════════════════════════════════════

QString DeductionEngine::generateFinalAudioDescription(const std::vector<DeductionSlot>& boardSlots)
{
    if (boardSlots.size() < 3) return "三个空槽尚未填满。";
    QStringList parts;
    for (size_t i = 0; i < boardSlots.size() && i < 3; ++i) {
        auto* s = findSample(boardSlots[i].sampleId);
        if (!s) continue;
        QString slotName = (i == 0) ? "牺牲" : (i == 1) ? "传递" : "定义";
        parts << QString("第%1槽[%2]：%3").arg(i + 1).arg(slotName).arg(s->name);
    }
    QString ret = "终局音频（唯一）由你推演出：\n" + parts.join("\n");
    ret += "\n你最终的决定是：" + generatePersonalityProfile().verdict + "。";
    return ret;
}
