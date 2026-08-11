#include "EchoEngine.h"

EchoEngine::EchoEngine(QObject* parent)
    : QObject(parent)
{
    m_mentorMessages = {
        "欢迎来到回声共鸣的世界，{name}！",
        "在这个实验室里，你将探索声音的奥秘。",
        "记住，声音不仅仅是听觉，它还可以影响我们的情绪和思维。",
        "在地下档案馆中，你会发现许多关于声音的秘密。",
        "家，是我们心灵的港湾，也是声音最温暖的地方。"
    };
}

QString EchoEngine::sceneName() const
{
    switch (m_currentAct) {
    case 1: return "第一幕：实验室";
    case 2: return "第二幕：地下档案馆";
    case 3: return "第三幕：家";
    default: return "未知场景";
    }
}

QString EchoEngine::getMentorMessage(int index) const
{
    if (index < 0 || index >= m_mentorMessages.size())
        return "";
    QString msg = m_mentorMessages[index];
    QString name = m_playerName.isEmpty() ? "小周" : m_playerName;
    msg.replace("{name}", name);
    return msg;
}
