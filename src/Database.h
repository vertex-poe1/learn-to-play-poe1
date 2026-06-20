#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <sqlite3.h>

class Database
{
public:
    explicit Database(const QString &path);
    ~Database();

    bool    isOpen()    const { return m_db != nullptr; }
    QString lastError() const { return m_lastError; }
    QString path()      const { return m_path; }

    struct InstallState {
        qint64 id{-1};
        qint64 fileCreatedAt{0};
        qint64 fileModifiedAt{0};
        qint64 fileSize{0};
        qint64 lastByteOffset{0};
    };

    struct WhisperRecord {
        QString direction;   // "from" or "to"
        QString playerName;
        QString message;
        QString occurredAt;  // "YYYY-MM-DD HH:MM:SS"
    };

    // Inserts the install path if new; returns current state either way.
    InstallState upsertInstall(const QString &installPath);

    // Returns all whispers ordered by time; optionally filtered to one player.
    QList<WhisperRecord> fetchWhispers(const QString &playerFilter = {}) const;

    // Returns distinct whisper partner names, ordered by most-recent message.
    QStringList fetchWhisperPartners() const;

private:
    void applyPragmas();
    void initSchema();
    void migrate(int fromVersion);

    sqlite3 *m_db{nullptr};
    QString  m_path;
    QString  m_lastError;
};
