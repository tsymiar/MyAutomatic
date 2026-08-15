#pragma once
#include "CommonFragment.h"
#include <QObject>
#include <QMap>
#include <QStringList>

class EchoEngine : public QObject {
    Q_OBJECT
public:
    explicit EchoEngine(QObject* parent = nullptr);
    ~EchoEngine();

    // ── 章节管理 ──
    void generateFragments(GameChapter chapter);
    GameChapter currentChapter() const { return m_currentChapter; }
    QString chapterName() const;
    QString chapterDescription() const;
    int chapterNumber() const { return static_cast<int>(m_currentChapter); }

    // ── 碎片管理 ──
    const std::vector<CommonFragment>& fragments() const { return m_fragments; }
    std::vector<CommonFragment>& fragments() { return m_fragments; }
    const std::vector<TimelineSlot>& timeline() const { return m_timeline; }
    bool placeFragment(int fragmentId, int slotIndex);
    bool removeFragment(int slotIndex);
    float calculateCoherence();
    bool isDistorted() const { return m_isDistorted; }
    int totalFragments() const { return static_cast<int>(m_fragments.size()); }
    int placedFragmentCount() const;
    int timelineSlotCount() const { return static_cast<int>(m_timeline.size()); }
    bool isTimelineFull() const;
    void resetCurrentChapter();

    // ── 锚点音系统 ──
    AnchorType currentAnchor() const { return m_currentAnchor; }
    bool isAnchorCorrect() const;
    void setOfficialAnchor(bool official) { m_useOfficialAnchor = official; }
    bool useOfficialAnchor() const { return m_useOfficialAnchor; }

    // ── 逆向播放 ──
    bool isReverseUnlocked() const { return m_reverseUnlocked; }
    void unlockReverse();
    void reverseFragment(int fragmentId);
    bool isFragmentReversed(int fragmentId) const;
    QString getReverseDescription(int fragmentId) const;

    // ── 偏执/焦虑系统 ──
    float paranoiaLevel() const { return m_paranoiaLevel; }
    float anxietyLevel() const { return m_anxietyLevel; }
    void adjustParanoia(float delta);
    void adjustAnxiety(float delta);
    QString getLowFreqWarning() const;
    QString getHallucinationText() const;

    // ── 角色系统 ──
    const std::vector<CharacterInfo>& characters() const { return m_characters; }
    void revealCharacter(int index);
    QString getCharacterDialogue(const QString& name, int lineIndex) const;

    // ── 导师信息 ──
    QString getMentorMessage(int index) const;
    void setPlayerName(const QString& name) { m_playerName = name; }
    // 玩家显示名：空则用默认名（唯一默认名来源）
    QString playerDisplayName() const { return m_playerName.isEmpty() ? DEFAULT_PLAYER_NAME : m_playerName; }
    // 将文案中的 {name} 占位符替换为玩家显示名
    QString applyPlayerName(const QString& text) const;
    QString playerName() const { return m_playerName; }

    // ── 选择系统 ──
    void recordChoice(PlayerChoice choice);
    const std::vector<ChoiceRecord>& choices() const { return m_choices; }
    bool hasChosen(PlayerChoice choice) const;
    PlayerChoice getChapterChoice(GameChapter chapter) const;

    // ── 结局系统 ──
    EndingType calculateEnding() const;
    EndingInfo getEndingInfo(EndingType type) const;
    int unlockedEndingCount() const;
    int totalEndingCount() const { return static_cast<int>(m_endings.size()); }
    QString endingTitle() const;
    QString endingDescription() const;
    bool isEndingUnlocked(EndingType type) const;
    void unlockEnding(EndingType type);

    // ── 底噪意识 ──
    float lowFreqIntensity() const { return m_lowFreqIntensity; }
    void increaseLowFreq();
    bool canCommuneWithNoise() const { return m_lowFreqIntensity > 0.7f; }
    QString noiseDialogue() const;

    // ── 摩斯电码 ──
    QString getMorseMessage() const;
    bool isMorseActive() const { return m_morseActive; }
    void activateMorse() { m_morseActive = true; }

    // ── 难度 ──
    bool isHardMode() const { return m_hardMode; }
    void setHardMode(bool hard) { m_hardMode = hard; }

    // ── 共鸣度系统（耳蜗共鸣）──
    float resonanceLevel() const { return m_resonanceLevel; }
    void addResonance(float delta);
    bool isResonancePeaked() const { return m_resonanceLevel >= 1.0f; }
    bool isFirstSoundUnlocked() const { return m_firstSoundUnlocked; }
    void unlockFirstSound() { m_firstSoundUnlocked = true; }

    // ── 回声碎片（支线）──
    const std::vector<EchoFragment>& echoFragments() const { return m_echoFragments; }
    void unlockEchoFragment(int index);
    // 章节完成时按进度解锁对应回声碎片（接入支线叙事）
    void unlockEchoFragmentsForChapter(GameChapter chapter);
    bool isEchoFragmentUnlocked(int index) const;
    int unlockedEchoFragmentCount() const;
    QString forgottenListText() const;

    // ── 结局余波 ──
    QString getEpilogue(EndingType type) const;

    // ── 何悦分支 ──
    HeYueState heYueState() const { return m_heYueState; }
    void setHeYueState(HeYueState s) { m_heYueState = s; }
    bool isHeYueRescued() const { return m_heYueState == HeYueState::Rescued; }
    bool isHeYueBrainwashed() const { return m_heYueState == HeYueState::Brainwashed; }

    // ── 陈远山"静音惩罚"（被静音的人）──
    // 放置伪造碎片累积"静音倾向"，达到阈值触发单侧静音视觉
    float silenceTendency() const { return m_silenceTendency; }
    bool isRightEarSilenced() const { return m_rightEarSilenced; }
    void resetSilencePunishment();

    // ── 何悦"绝对参考系"（干净声纹照妖镜）──
    // 救出何悦后，伪造碎片会被照出原形（可信度降级）
    bool isCredibilityExposed(const CommonFragment& f) const;

    // ── 碎片回声（衰减回声）──
    const std::vector<FragmentEcho>& echoes() const { return m_echoes; }
    void updateEchoes(float dt);

    // ── 老刘静默摩斯（沉默中浮现）──
    void updateSilenceMorse(float idleSeconds);
    QString getSilenceMorseReveal() const;
    bool isSilenceMorseReady() const;

    // ── 共鸣度与结局联动 ──
    // 共鸣度影响"被遗忘者名单"是否完整呈现

    // ── 动态文案组合（减少硬编码）──
    // 根据玩家名字、何悦状态、老刘沉默、静音惩罚等动态拼装选择提示
    struct ChoiceContent { QString prompt; QStringList options; };
    ChoiceContent buildChoice(ChoicePoint point) const;
    // 根据当前状态生成"底噪意识"的动态称呼与语气
    QString noiseConsciousnessLine() const;

    // ── 工具 ──
    static QColor credibilityToSpectrumColor(Credibility c);

signals:
    void fragmentPlaced(int fragmentId, int slotIndex);
    void fragmentRemoved(int fragmentId, int slotIndex);
    void sceneCompleted(int chapter, bool isDistorted);
    void paranoiaChanged(float level);
    void anxietyChanged(float level);
    void hallucinationTriggered(const QString& message);
    void reverseUnlocked();
    void choiceRequired(const QString& prompt, const QStringList& options);
    void endingReached(EndingType type);
    void morseMessage(const QString& message);
    void noiseCommunion(const QString& message);
    void silenceMode(bool active);

private:
    void initFragmentTypes();
    CommonFragment createFragment(FragmentType type, Credibility cred, float duration,
        bool isAnchor = false, AnchorType anchor = AnchorType::None);
    void setupChapterPrologue();
    void setupChapter1();
    void setupChapter2();
    void setupChapter3();
    void setupChapter4();
    void setupFinale();
    void initCharacters();
    void initEndings();
    void initMentorMessages();
    void initHallucinations();
    void initNoiseDialogues();
    void initMorseMessages();
    void initEchoFragments();
    void initEpilogues();

    std::vector<CommonFragment> m_fragments;
    std::vector<TimelineSlot> m_timeline;
    GameChapter m_currentChapter = GameChapter::Prologue;
    float m_paranoiaLevel = 0.3f;
    float m_anxietyLevel = 0.1f;
    float m_lowFreqIntensity = 0.05f;
    bool m_isDistorted = false;
    bool m_reverseUnlocked = false;
    bool m_hardMode = false;
    bool m_morseActive = false;
    bool m_silenceTriggered = false;
    bool m_useOfficialAnchor = true;
    AnchorType m_currentAnchor = AnchorType::None;
    int m_nextId = 0;

    QStringList m_mentorMessages;
    QStringList m_hallucinations;
    QStringList m_noiseDialogues;
    QStringList m_morseMessages;
    QString m_playerName;
    std::vector<CharacterInfo> m_characters;
    std::vector<ChoiceRecord> m_choices;
    QMap<EndingType, EndingInfo> m_endings;
    QMap<QString, QStringList> m_characterDialogues;
    std::vector<EchoFragment> m_echoFragments;
    std::vector<ForgottenEntry> m_forgottenEntries;
    QMap<EndingType, EpilogueRecord> m_epilogues;
    float m_resonanceLevel = 0.0f;
    bool m_firstSoundUnlocked = false;
    HeYueState m_heYueState = HeYueState::Unknown;

    // ── 巧妙机制内部状态 ──
    float m_silenceTendency = 0.0f;         // 静音倾向（放置伪造碎片累积）
    bool m_rightEarSilenced = false;        // 右耳被静音
    std::vector<FragmentEcho> m_echoes;     // 衰减回声
    QMap<int, int> m_fragmentPlaceCount;    // 碎片放置次数（共振）
    float m_silenceIdleAccum = 0.0f;        // 静默累计时长
    SilenceMorse m_silenceMorse;            // 老刘静默摩斯
    bool m_silenceMorseRevealed = false;    // 是否已浮现
};
