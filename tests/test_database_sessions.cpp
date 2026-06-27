#include <QTemporaryDir>
#include <QtTest>
#include <sqlite3.h>

#include "db/Database.h"

static qint64 insertInstall(sqlite3 *db, const char *path)
{
    char sql[512];
    std::snprintf(sql, sizeof(sql),
        "INSERT INTO installs(path) VALUES('%s');", path);
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    return sqlite3_last_insert_rowid(db);
}

static void insertSession(sqlite3 *db, qint64 installId,
    const char *startedAt, const char *endedAt = nullptr,
    int totalSecs = -1, int activeSecs = -1)
{
    char sql[512];
    if (endedAt && totalSecs >= 0 && activeSecs >= 0)
        std::snprintf(sql, sizeof(sql),
            "INSERT INTO sessions(install_id, started_at, ended_at, total_secs, active_secs) "
            "VALUES(%lld, '%s', '%s', %d, %d);",
            static_cast<long long>(installId), startedAt, endedAt, totalSecs, activeSecs);
    else if (endedAt)
        std::snprintf(sql, sizeof(sql),
            "INSERT INTO sessions(install_id, started_at, ended_at) "
            "VALUES(%lld, '%s', '%s');",
            static_cast<long long>(installId), startedAt, endedAt);
    else
        std::snprintf(sql, sizeof(sql),
            "INSERT INTO sessions(install_id, started_at) VALUES(%lld, '%s');",
            static_cast<long long>(installId), startedAt);
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

class DatabaseSessionsTest : public QObject
{
    Q_OBJECT
private slots:
    void emptyDb();
    void singleOpenSession();
    void singleClosedSession();
    void multipleSessionsChronologicalOrder();
    void limitCapsResults();
    void offsetSkipsNewest();
};

// Initialise schema via Database, then return a raw sqlite3 handle for inserts.
// Caller owns the handle and must call sqlite3_close.
static sqlite3 *openForInsert(const QString &path)
{
    { Database init(path); Q_ASSERT(init.isOpen()); }
    sqlite3 *raw = nullptr;
    sqlite3_open(path.toUtf8().constData(), &raw);
    return raw;
}

void DatabaseSessionsTest::emptyDb()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    Database db(tmp.path() + "/test.db");
    QVERIFY2(db.isOpen(), qPrintable(db.lastError()));
    QVERIFY(db.fetchSessions().isEmpty());
}

void DatabaseSessionsTest::singleOpenSession()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/test.db";

    sqlite3 *raw = openForInsert(path);
    qint64 iid = insertInstall(raw, "/game/Client.txt");
    insertSession(raw, iid, "2024-01-15 10:00:00");
    sqlite3_close(raw);

    Database db(path);
    const auto sessions = db.fetchSessions();
    QCOMPARE(sessions.size(), 1);
    QCOMPARE(sessions[0].startedAt,  QString("2024-01-15 10:00:00"));
    QVERIFY(sessions[0].endedAt.isEmpty());
    QCOMPARE(sessions[0].totalSecs,  -1);
    QCOMPARE(sessions[0].activeSecs, -1);
    QCOMPARE(sessions[0].installPath, QString("/game/Client.txt"));
}

void DatabaseSessionsTest::singleClosedSession()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/test.db";

    sqlite3 *raw = openForInsert(path);
    qint64 iid = insertInstall(raw, "/game/Client.txt");
    insertSession(raw, iid, "2024-01-15 10:00:00", "2024-01-15 12:00:00", 7200, 6500);
    sqlite3_close(raw);

    Database db(path);
    const auto sessions = db.fetchSessions();
    QCOMPARE(sessions.size(), 1);
    QCOMPARE(sessions[0].startedAt,  QString("2024-01-15 10:00:00"));
    QCOMPARE(sessions[0].endedAt,    QString("2024-01-15 12:00:00"));
    QCOMPARE(sessions[0].totalSecs,  7200);
    QCOMPARE(sessions[0].activeSecs, 6500);
    QCOMPARE(sessions[0].installPath, QString("/game/Client.txt"));
}

void DatabaseSessionsTest::multipleSessionsChronologicalOrder()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/test.db";

    sqlite3 *raw = openForInsert(path);
    qint64 iid = insertInstall(raw, "/game/Client.txt");
    insertSession(raw, iid, "2024-01-15 10:00:00", "2024-01-15 11:00:00");
    insertSession(raw, iid, "2024-01-15 14:00:00", "2024-01-15 16:00:00");
    insertSession(raw, iid, "2024-01-16 09:00:00");
    sqlite3_close(raw);

    Database db(path);
    const auto sessions = db.fetchSessions();
    QCOMPARE(sessions.size(), 3);
    // Must come back oldest-first (fetchSessions fetches DESC then reverses).
    QVERIFY(sessions[0].startedAt < sessions[1].startedAt);
    QVERIFY(sessions[1].startedAt < sessions[2].startedAt);
    QCOMPARE(sessions[0].startedAt, QString("2024-01-15 10:00:00"));
    QCOMPARE(sessions[1].startedAt, QString("2024-01-15 14:00:00"));
    QCOMPARE(sessions[2].startedAt, QString("2024-01-16 09:00:00"));
}

void DatabaseSessionsTest::limitCapsResults()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/test.db";

    sqlite3 *raw = openForInsert(path);
    qint64 iid = insertInstall(raw, "/game/Client.txt");
    insertSession(raw, iid, "2024-01-01 10:00:00", "2024-01-01 11:00:00");
    insertSession(raw, iid, "2024-01-02 10:00:00", "2024-01-02 11:00:00");
    insertSession(raw, iid, "2024-01-03 10:00:00", "2024-01-03 11:00:00");
    sqlite3_close(raw);

    Database db(path);
    // limit=2: fetches 2 newest DESC → [Jan-03, Jan-02], reversed → [Jan-02, Jan-03]
    const auto sessions = db.fetchSessions(2);
    QCOMPARE(sessions.size(), 2);
    QCOMPARE(sessions[0].startedAt, QString("2024-01-02 10:00:00"));
    QCOMPARE(sessions[1].startedAt, QString("2024-01-03 10:00:00"));
}

void DatabaseSessionsTest::offsetSkipsNewest()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/test.db";

    sqlite3 *raw = openForInsert(path);
    qint64 iid = insertInstall(raw, "/game/Client.txt");
    insertSession(raw, iid, "2024-01-01 10:00:00", "2024-01-01 11:00:00");
    insertSession(raw, iid, "2024-01-02 10:00:00", "2024-01-02 11:00:00");
    insertSession(raw, iid, "2024-01-03 10:00:00", "2024-01-03 11:00:00");
    sqlite3_close(raw);

    Database db(path);
    // offset=1: skips Jan-03 (newest) → returns [Jan-02, Jan-01] DESC, reversed → [Jan-01, Jan-02]
    const auto sessions = db.fetchSessions(0, 1);
    QCOMPARE(sessions.size(), 2);
    QCOMPARE(sessions[0].startedAt, QString("2024-01-01 10:00:00"));
    QCOMPARE(sessions[1].startedAt, QString("2024-01-02 10:00:00"));
}

QTEST_GUILESS_MAIN(DatabaseSessionsTest)
#include "test_database_sessions.moc"
