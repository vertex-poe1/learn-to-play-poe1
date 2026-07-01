package query

import (
	"database/sql"
	"strings"
	"testing"
)

// Minimal schema covering the tables used by the query functions under test.
const testSchema = `
CREATE TABLE installs (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT NOT NULL UNIQUE
);
CREATE TABLE accounts (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL
);
CREATE TABLE classes (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL
);
CREATE TABLE characters (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    class_id INTEGER REFERENCES classes(id),
    name     TEXT NOT NULL
);
CREATE TABLE sessions (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    install_id  INTEGER NOT NULL REFERENCES installs(id),
    account_id  INTEGER REFERENCES accounts(id),
    char_id     INTEGER REFERENCES characters(id),
    started_at  TEXT NOT NULL,
    ended_at    TEXT,
    total_secs  INTEGER,
    active_secs INTEGER
);
CREATE TABLE chats (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    public_char_id INTEGER NOT NULL DEFAULT 0,
    guild_id       INTEGER,
    channel        TEXT NOT NULL,
    message        TEXT NOT NULL,
    occurred_at    TEXT NOT NULL
);
CREATE TABLE whispers (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    guild_id    INTEGER,
    direction   TEXT NOT NULL,
    player_name TEXT NOT NULL,
    message     TEXT NOT NULL,
    occurred_at TEXT NOT NULL
);
`

func openTestDB(t *testing.T) *DB {
	t.Helper()
	// Each test gets its own named in-memory DB so there is no cross-test contamination.
	name := strings.ReplaceAll(t.Name(), "/", "_")
	raw, err := sql.Open("sqlite", "file:"+name+"?mode=memory&cache=shared")
	if err != nil {
		t.Fatalf("open in-memory db: %v", err)
	}
	raw.SetMaxOpenConns(1)
	if _, err := raw.Exec(testSchema); err != nil {
		raw.Close()
		t.Fatalf("apply schema: %v", err)
	}
	t.Cleanup(func() { raw.Close() })
	return &DB{db: raw}
}

func mustExec(t *testing.T, db *DB, q string, args ...any) {
	t.Helper()
	if _, err := db.db.Exec(q, args...); err != nil {
		t.Fatalf("exec %q: %v", q, err)
	}
}

func insertInstall(t *testing.T, db *DB, path string) int64 {
	t.Helper()
	res, err := db.db.Exec("INSERT INTO installs(path) VALUES(?)", path)
	if err != nil {
		t.Fatalf("insertInstall: %v", err)
	}
	id, _ := res.LastInsertId()
	return id
}

func insertSession(t *testing.T, db *DB, installID int64, startedAt, endedAt string, totalSecs, activeSecs int) {
	t.Helper()
	if endedAt == "" {
		mustExec(t, db, "INSERT INTO sessions(install_id,started_at) VALUES(?,?)", installID, startedAt)
	} else if totalSecs < 0 {
		mustExec(t, db, "INSERT INTO sessions(install_id,started_at,ended_at) VALUES(?,?,?)", installID, startedAt, endedAt)
	} else {
		mustExec(t, db, "INSERT INTO sessions(install_id,started_at,ended_at,total_secs,active_secs) VALUES(?,?,?,?,?)",
			installID, startedAt, endedAt, totalSecs, activeSecs)
	}
}

// ── FetchSessions ─────────────────────────────────────────────────────────────

func TestFetchSessions_empty(t *testing.T) {
	db := openTestDB(t)
	sessions, err := db.FetchSessions(0, 0)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 0 {
		t.Fatalf("expected 0 sessions, got %d", len(sessions))
	}
}

func TestFetchSessions_singleOpen(t *testing.T) {
	db := openTestDB(t)
	iid := insertInstall(t, db, "/game/Client.txt")
	insertSession(t, db, iid, "2024-01-15 10:00:00", "", -1, -1)

	sessions, err := db.FetchSessions(0, 0)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 1 {
		t.Fatalf("expected 1 session, got %d", len(sessions))
	}
	s := sessions[0]
	if s.StartedAt != "2024-01-15 10:00:00" {
		t.Errorf("StartedAt: got %q, want %q", s.StartedAt, "2024-01-15 10:00:00")
	}
	if s.EndedAt != "" {
		t.Errorf("EndedAt: got %q, want empty", s.EndedAt)
	}
	if s.TotalSecs != -1 {
		t.Errorf("TotalSecs: got %d, want -1", s.TotalSecs)
	}
	if s.ActiveSecs != -1 {
		t.Errorf("ActiveSecs: got %d, want -1", s.ActiveSecs)
	}
	if s.InstallPath != "/game/Client.txt" {
		t.Errorf("InstallPath: got %q, want %q", s.InstallPath, "/game/Client.txt")
	}
}

func TestFetchSessions_singleClosed(t *testing.T) {
	db := openTestDB(t)
	iid := insertInstall(t, db, "/game/Client.txt")
	insertSession(t, db, iid, "2024-01-15 10:00:00", "2024-01-15 12:00:00", 7200, 6500)

	sessions, err := db.FetchSessions(0, 0)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 1 {
		t.Fatalf("expected 1 session, got %d", len(sessions))
	}
	s := sessions[0]
	if s.EndedAt != "2024-01-15 12:00:00" {
		t.Errorf("EndedAt: got %q, want %q", s.EndedAt, "2024-01-15 12:00:00")
	}
	if s.TotalSecs != 7200 {
		t.Errorf("TotalSecs: got %d, want 7200", s.TotalSecs)
	}
	if s.ActiveSecs != 6500 {
		t.Errorf("ActiveSecs: got %d, want 6500", s.ActiveSecs)
	}
}

func TestFetchSessions_chronologicalOrder(t *testing.T) {
	db := openTestDB(t)
	iid := insertInstall(t, db, "/game/Client.txt")
	insertSession(t, db, iid, "2024-01-15 10:00:00", "2024-01-15 11:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-15 14:00:00", "2024-01-15 16:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-16 09:00:00", "", -1, -1)

	sessions, err := db.FetchSessions(0, 0)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 3 {
		t.Fatalf("expected 3 sessions, got %d", len(sessions))
	}
	if sessions[0].StartedAt >= sessions[1].StartedAt || sessions[1].StartedAt >= sessions[2].StartedAt {
		t.Errorf("sessions not in chronological order: %v", []string{
			sessions[0].StartedAt, sessions[1].StartedAt, sessions[2].StartedAt,
		})
	}
	if sessions[0].StartedAt != "2024-01-15 10:00:00" {
		t.Errorf("sessions[0].StartedAt = %q, want 2024-01-15 10:00:00", sessions[0].StartedAt)
	}
	if sessions[2].StartedAt != "2024-01-16 09:00:00" {
		t.Errorf("sessions[2].StartedAt = %q, want 2024-01-16 09:00:00", sessions[2].StartedAt)
	}
}

func TestFetchSessions_limitCaps(t *testing.T) {
	db := openTestDB(t)
	iid := insertInstall(t, db, "/game/Client.txt")
	insertSession(t, db, iid, "2024-01-01 10:00:00", "2024-01-01 11:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-02 10:00:00", "2024-01-02 11:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-03 10:00:00", "2024-01-03 11:00:00", -1, -1)

	// limit=2: fetches 2 newest DESC → [Jan-03, Jan-02], reversed → [Jan-02, Jan-03]
	sessions, err := db.FetchSessions(2, 0)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 2 {
		t.Fatalf("expected 2 sessions, got %d", len(sessions))
	}
	if sessions[0].StartedAt != "2024-01-02 10:00:00" {
		t.Errorf("sessions[0].StartedAt = %q, want 2024-01-02", sessions[0].StartedAt)
	}
	if sessions[1].StartedAt != "2024-01-03 10:00:00" {
		t.Errorf("sessions[1].StartedAt = %q, want 2024-01-03", sessions[1].StartedAt)
	}
}

func TestFetchSessions_offsetSkipsNewest(t *testing.T) {
	db := openTestDB(t)
	iid := insertInstall(t, db, "/game/Client.txt")
	insertSession(t, db, iid, "2024-01-01 10:00:00", "2024-01-01 11:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-02 10:00:00", "2024-01-02 11:00:00", -1, -1)
	insertSession(t, db, iid, "2024-01-03 10:00:00", "2024-01-03 11:00:00", -1, -1)

	// offset=1: skips Jan-03 (newest) → returns [Jan-02, Jan-01] DESC, reversed → [Jan-01, Jan-02]
	sessions, err := db.FetchSessions(0, 1)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(sessions) != 2 {
		t.Fatalf("expected 2 sessions, got %d", len(sessions))
	}
	if sessions[0].StartedAt != "2024-01-01 10:00:00" {
		t.Errorf("sessions[0].StartedAt = %q, want 2024-01-01", sessions[0].StartedAt)
	}
	if sessions[1].StartedAt != "2024-01-02 10:00:00" {
		t.Errorf("sessions[1].StartedAt = %q, want 2024-01-02", sessions[1].StartedAt)
	}
}

// ── FetchChatDates ────────────────────────────────────────────────────────────

func insertChat(t *testing.T, db *DB, channel, occurredAt string) {
	t.Helper()
	mustExec(t, db, "INSERT INTO chats(channel,message,occurred_at) VALUES(?,?,?)",
		channel, "msg", occurredAt)
}

func insertWhisper(t *testing.T, db *DB, playerName, occurredAt string) {
	t.Helper()
	mustExec(t, db, "INSERT INTO whispers(direction,player_name,message,occurred_at) VALUES(?,?,?,?)",
		"from", playerName, "msg", occurredAt)
}

func TestFetchChatDates_noFilter(t *testing.T) {
	db := openTestDB(t)
	insertChat(t, db, "#", "2024-03-10 12:00:00")
	dates, err := db.FetchChatDates(nil, false)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if dates != nil {
		t.Errorf("expected nil for empty filter, got %v", dates)
	}
}

func TestFetchChatDates_chatOnly(t *testing.T) {
	db := openTestDB(t)
	insertChat(t, db, "#", "2024-03-10 12:00:00")
	insertChat(t, db, "#", "2024-03-10 14:00:00") // same date, should deduplicate
	insertChat(t, db, "#", "2024-03-11 09:00:00")
	insertChat(t, db, "$", "2024-03-12 08:00:00") // different channel, excluded

	dates, err := db.FetchChatDates([]string{"#"}, false)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(dates) != 2 {
		t.Fatalf("expected 2 dates, got %d: %v", len(dates), dates)
	}
	// most-recent first
	if dates[0] != "2024-03-11" {
		t.Errorf("dates[0] = %q, want 2024-03-11", dates[0])
	}
	if dates[1] != "2024-03-10" {
		t.Errorf("dates[1] = %q, want 2024-03-10", dates[1])
	}
}

func TestFetchChatDates_dmsOnly(t *testing.T) {
	db := openTestDB(t)
	insertWhisper(t, db, "Alice", "2024-03-05 10:00:00")
	insertWhisper(t, db, "Bob", "2024-03-07 11:00:00")

	dates, err := db.FetchChatDates(nil, true)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(dates) != 2 {
		t.Fatalf("expected 2 dates, got %d: %v", len(dates), dates)
	}
	if dates[0] != "2024-03-07" {
		t.Errorf("dates[0] = %q, want 2024-03-07", dates[0])
	}
	if dates[1] != "2024-03-05" {
		t.Errorf("dates[1] = %q, want 2024-03-05", dates[1])
	}
}

func TestFetchChatDates_combined(t *testing.T) {
	db := openTestDB(t)
	insertChat(t, db, "#", "2024-03-10 12:00:00")
	insertWhisper(t, db, "Alice", "2024-03-11 10:00:00")
	insertWhisper(t, db, "Bob", "2024-03-10 09:00:00") // same date as chat, should deduplicate

	dates, err := db.FetchChatDates([]string{"#"}, true)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(dates) != 2 {
		t.Fatalf("expected 2 dates, got %d: %v", len(dates), dates)
	}
	if dates[0] != "2024-03-11" {
		t.Errorf("dates[0] = %q, want 2024-03-11", dates[0])
	}
	if dates[1] != "2024-03-10" {
		t.Errorf("dates[1] = %q, want 2024-03-10", dates[1])
	}
}

// ── FetchWhisperPartnersWithDates ─────────────────────────────────────────────

func TestFetchWhisperPartnersWithDates_empty(t *testing.T) {
	db := openTestDB(t)
	partners, err := db.FetchWhisperPartnersWithDates()
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if partners != nil {
		t.Errorf("expected nil for empty table, got %v", partners)
	}
}

func TestFetchWhisperPartnersWithDates_single(t *testing.T) {
	db := openTestDB(t)
	insertWhisper(t, db, "Alice", "2024-03-10 12:00:00")
	insertWhisper(t, db, "Alice", "2024-03-11 09:00:00")
	insertWhisper(t, db, "Alice", "2024-03-10 15:00:00") // same date as first, deduplicates

	partners, err := db.FetchWhisperPartnersWithDates()
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(partners) != 1 {
		t.Fatalf("expected 1 partner, got %d", len(partners))
	}
	p := partners[0]
	if p.Name != "Alice" {
		t.Errorf("Name = %q, want Alice", p.Name)
	}
	if len(p.Dates) != 2 {
		t.Fatalf("expected 2 dates, got %d: %v", len(p.Dates), p.Dates)
	}
	// most-recent date first
	if p.Dates[0] != "2024-03-11" {
		t.Errorf("Dates[0] = %q, want 2024-03-11", p.Dates[0])
	}
	if p.Dates[1] != "2024-03-10" {
		t.Errorf("Dates[1] = %q, want 2024-03-10", p.Dates[1])
	}
}

func TestFetchWhisperPartnersWithDates_orderedByRecency(t *testing.T) {
	db := openTestDB(t)
	// Alice messaged first, Bob most recently — Bob should come first.
	insertWhisper(t, db, "Alice", "2024-03-01 10:00:00")
	insertWhisper(t, db, "Bob", "2024-03-15 10:00:00")
	insertWhisper(t, db, "Alice", "2024-03-05 10:00:00")

	partners, err := db.FetchWhisperPartnersWithDates()
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(partners) != 2 {
		t.Fatalf("expected 2 partners, got %d", len(partners))
	}
	if partners[0].Name != "Bob" {
		t.Errorf("partners[0].Name = %q, want Bob (most recent)", partners[0].Name)
	}
	if partners[1].Name != "Alice" {
		t.Errorf("partners[1].Name = %q, want Alice", partners[1].Name)
	}
	if len(partners[1].Dates) != 2 {
		t.Errorf("Alice should have 2 distinct dates, got %d", len(partners[1].Dates))
	}
}
