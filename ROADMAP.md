<!-- ROADMAP.md (markdown) -->

# Roadmap

- [ ] Public release (first public build shipped to users)
- [ ] Investigate `replace_object` log lines as a source of in-map events
- [ ] Investigate `window` "Lost focus" / "Gained focus" lines as AFK or session-pause signals
- [ ] Auto start on boot (Windows registry `HKCU\…\Run`; Linux `.desktop` autostart)
- [ ] Companion mode: web API only, no overlay, no PoE install required
- [ ] macOS overlay (`NSWindow` level + `ignoresMouseEvents`; needs PoE Mac client testing)
- [ ] Game overlay interactive content beyond proof-of-concept text
- [ ] Historical events panel: virtual scrolling via QListView + QAbstractItemModel + QStyledItemDelegate (replaces load-N-at-a-time approach; delegate ports existing custom-paint logic from NotificationWidget; enables millions of rows with no memory growth)
- [ ] Chat/whisper panel: three views — whispers-only, chats-only, and a combined view (UNION ALL of both tables with a source column to distinguish them); chats are not in the events spine so the combined view is its own dedicated query, not a filter on events
