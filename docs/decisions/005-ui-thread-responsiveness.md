<!-- docs/decisions/005-ui-thread-responsiveness.md (markdown) -->

# ADR-005: The UI Thread Has a Deadline

**Status**: Decided  
**Date**: 2026-06-22  
**Deciders**: MovingCairn

---

## Context

This application combines a companion window with a transparent, always-on-top overlay that renders over the game. The overlay is not decorative — it delivers information the player acts on while playing. Its correctness condition is: it must remain visually present, accurately positioned, and click-through at all times during gameplay.

Windows enforces this condition implicitly and ruthlessly. If the application's message loop does not respond within a few seconds, Windows marks the window "not responding" and withdraws its ability to receive or pass through input. For the overlay, that is not a degraded experience — it is a game-blocking failure. The player cannot click through the overlay, cannot interact with the game, and must force-close or restart the application to recover. This has happened, more than once, in development.

The root cause of every incident has been the same: **work that blocks the UI thread's event loop.** Synchronous SQLite reads called from page rebuild slots. Win32 window enumeration running inline on the poll timer. A database query without an index that scans the full table before returning. In each case the operation was individually reasonable — a database call, a system call — and in each case it stalled the event loop long enough for Windows to revoke click-through.

The pattern is self-reproducing. A new feature adds a database call to a new page's rebuild slot, or adds work to an existing timer, and the class reappears. Not because the developer forgot the last fix, but because the **design makes the wrong path easy and the right path something you have to remember to choose.**

This ADR names the constraint, classifies it correctly, and establishes the rules that make the right path the default.

---

## The core reframe

UI-thread blocking has historically been treated as a *performance problem*: something to optimize when it becomes noticeable. That classification is wrong for this application, and the misclassification is why fixes don't stick.

**A UI-thread stall that freezes the overlay is a correctness bug, not a performance bug.** It makes the game unplayable for the duration. Its severity class is the same as a crash. It does not belong in a perf backlog; it is a blocking defect.

This reframe has practical consequences. A performance problem can be deferred. A correctness invariant cannot. "We'll async-ify it later if it gets slow" is a reasonable response to a perf concern; it is not a reasonable response to a constraint that makes the overlay fail.

---

## Decision

### The governing principle

> **The UI thread renders and dispatches; it never waits. Any operation whose duration cannot be bounded to a few milliseconds belongs off the main thread, or it does not run on a frame.**

The short form: **the main thread has a deadline.**

### Feature decision checklist

When building any feature that involves page rebuilds, timers, event-bus callbacks, overlay updates, or database access, answer these questions before writing the implementation:

1. **Where does this work run?** If the answer is "on the UI thread," proceed to question 2. If the answer is "off-thread, result delivered via signal," you are done.
2. **Is the duration of this work bounded, independent of external state?** "Bounded" means: not affected by database row count, file size, number of running processes, number of open windows, or any other runtime variable. A fixed-cost layout operation is bounded; a query over a growing table is not.
3. **If bounded: does it fit within the frame budget?** "A few milliseconds" is the right mental model; the frame-budget guards carry the specific per-callsite thresholds and are the living source of truth. If it fits, it may run on the UI thread. If not, move it off.
4. **If unbounded or over budget: it goes off-thread.** This is not optional. It is not "we'll revisit if it becomes slow." It is a precondition for the feature shipping.

### Hard limits

The following are unconditional. They are not performance guidelines. They are not subject to "it's usually fast enough." They are correctness constraints on an overlay that fails visibly and immediately when they are violated.

| What | Why it is unconditional |
|---|---|
| No synchronous SQLite query on the UI thread | Query cost scales with table size; tables grow; "fast now" is not a bound. One missing index converts a millisecond read into a full-table scan. This has caused production freezes. |
| No unbounded loop on the UI thread | Any loop whose iteration count is driven by external data — row count, file size, event count, window count — is unbounded by definition. Chunk it, budget it, or move it off-thread. |
| No blocking OS call on the UI thread | Win32 window/process enumeration, file I/O, registry access, network — none of it runs inline on a frame. OS calls have no guaranteed upper bound. |
| No blocking wait of any kind on the UI thread | No `QThread::wait()`, no mutex held across I/O, no busy-poll, no synchronous IPC. The main thread never parks. |
| No feature may starve or route around the overlay keepalive | The keepalive periodically re-asserts z-order and click-through state from outside the event loop. It is a backstop, not a fallback. Features that consume main-thread time must leave it room to fire. |

### The async path is the sanctioned path

The async read service (see ROADMAP) is being built. Until it lands, new synchronous database access on the UI thread is incurred as known, tracked debt — not as acceptable practice. Once it exists, it is the only sanctioned route to the database from a page or slot. Reaching past it to call a database method synchronously on the UI thread is the same kind of violation as reaching for QSS in a project that decided on native APIs (ADR-002): it works right now, it is wrong by the rules of this project, and it will be caught in review.

New database access — in new features, new pages, new slots — uses the async path from the start. Not as a later optimization. From the start.

---

## Consequences

- **The default question for any new page or feature is "where does this work run?" — answered before the feature is designed, not after it ships.** Async-by-default for data access; the main thread is presumed to be doing only render and dispatch until proven otherwise.

- **"We'll async-ify it later" is not an acceptable answer during design or review.** It is the statement that has preceded every UI-thread freeze in this project's history. It is recognized as such.

- **A PR that adds a synchronous database call or an unbounded loop to a UI-thread slot is a correctness regression, not a performance nit.** It is treated as a blocking finding in review, the same way a null dereference or an unhandled write failure would be.

- **The frame-budget tooling is a first-class development aid, not an optional debugging helper.** Every rebuild slot and timer carries a budget guard. When a guard fires, it is a signal — not noise — that something has violated the deadline. Silencing it is not acceptable.

- **New overlay features inherit the same constraints.** As the overlay gains interactive content, it will acquire its own data needs. Those follow the same rules: bounded work only on the main thread, everything else off-thread, no blocking.

- **This ADR is what a reviewer cites.** Without a decided constraint written down, a reviewer noting a synchronous DB call in a slot is asserting a personal preference. With it, they are enforcing a project decision. That citation function is the reason this document exists.

- **The specific implementation** — the async read service, the budget guard, the progress handler, the keepalive — is tracked in the ROADMAP and documented in the plan files. This ADR records the constraint and its classification, not the implementation. When the implementation evolves, this document does not need to change.
