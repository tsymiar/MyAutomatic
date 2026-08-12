#pragma once
#include "SoundFragment.h"
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
    const std::vector<SoundFragment>& fragments() const { return m_fragments; }
    std::vector<SoundFragment>& fragments() { return m_fragments; }
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
    QString endingTitle() const;
    QString endingDescription() const;
    bool isEndingUnlocked(EndingType type) const;

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
    SoundFragment createFragment(FragmentType type, Credibility cred, float duration,
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

    std::vector<SoundFragment> m_fragments;
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
};
