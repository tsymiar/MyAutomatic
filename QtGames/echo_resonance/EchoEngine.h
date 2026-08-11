#pragma once
#include "SoundFragment.h"
#include <QObject>
#include <QStringList>

class EchoEngine : public QObject {
    Q_OBJECT
public:
    explicit EchoEngine(QObject* parent = nullptr);

    void setPlayerName(const QString& name) { m_playerName = name; }
    QString playerName() const { return m_playerName; }

    int currentAct() const { return m_currentAct; }
    QString sceneName() const;
    QString getMentorMessage(int index) const;

signals:
    void nameConfirmed(const QString& name);

private:
    QStringList m_mentorMessages;
    QString m_playerName;
    int m_currentAct = 1;
};
