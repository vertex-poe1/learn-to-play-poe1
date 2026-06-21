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
        QString guildTag;    // may be empty
        QString message;
        QString occurredAt;  // "YYYY-MM-DD HH:MM:SS"
    };

    struct PartnerRecord {
        QString     name;
        QStringList dates; // distinct "YYYY-MM-DD" values, most-recent first
    };

    // Inserts the install path if new; returns current state either way.
    InstallState upsertInstall(const QString &installPath);

    // Returns whispers ordered by time; optionally filtered to one player.
    // limit > 0 returns only the most recent N messages.
    QList<WhisperRecord> fetchWhispers(const QString &playerFilter = {}, int limit = 0) const;

    // Returns distinct whisper partner names, ordered by most-recent message.
    QStringList fetchWhisperPartners() const;

    // Returns partners ordered by most-recent activity, each with their distinct
    // active dates (YYYY-MM-DD), most-recent first. Used for the filter menu buckets.
    QList<PartnerRecord> fetchWhisperPartnersWithDates() const;

private:
    void applyPragmas();
    void initSchema();
    void migrate(int fromVersion);

    sqlite3 *m_db{nullptr};
    QString  m_path;
    QString  m_lastError;
};
