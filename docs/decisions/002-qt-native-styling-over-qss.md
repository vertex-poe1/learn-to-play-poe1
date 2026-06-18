# ADR-002: Prefer Qt Native Styling APIs Over Qt Style Sheets (QSS)

**Status**: Decided  
**Date**: 2026-06-18  
**Deciders**: MovingCairn

---

## Context

Qt offers two distinct approaches to styling widgets:

1. **Qt Style Sheets (QSS)** — a CSS-like syntax applied as strings at runtime (e.g. `widget->setStyleSheet("QLabel { color: red; font-size: 14px; }")`). Qt parses this CSS internally and maps it onto widget properties.

2. **Qt native APIs** — direct C++ calls that set the same properties without a CSS parsing layer: `QPalette` for colors and brushes, `QFont` + `setFont()` for typography, `QStyle` / `QProxyStyle` for drawing overrides, `setFixedSize()` / `setContentsMargins()` for geometry, `QPainter` for fully custom widget rendering.

QSS was added in Qt 4 and has historically lagged behind the rest of the Qt API in terms of correctness, platform parity, and stability.

---

## Problem with QSS

**QSS is brittle across Qt versions.**  
QSS property support is inconsistent between Qt versions. Properties that work in Qt 6.4 may silently fail or render differently in Qt 6.7. Because QSS strings are parsed at runtime there is no compile-time check — breakage is invisible until the app is tested against a new Qt release. This is the same class of problem that makes browser CSS fragile: the spec evolves, parsers change, and assumptions encoded in strings rot.

**QSS mixes presentation logic into code as opaque strings.**  
A `setStyleSheet()` call embeds a CSS string that a C++ compiler cannot validate. Renaming a widget subclass or changing a widget hierarchy can silently stop a QSS rule from matching, with no warning.

**QSS introduces a "webby" mental model that doesn't match Qt's object model.**  
Qt is a retained-mode widget tree. Its native styling model (`QPalette`, `QStyle`) maps directly onto that tree and is well-documented, versioned, and tested. QSS creates a parallel styling layer with its own cascade and specificity rules that can conflict with and override the native layer in surprising ways.

**QSS limits performance options.**  
`QProxyStyle` and `QPainter`-based custom drawing are the correct paths for pixel-perfect custom rendering (e.g. overlay elements that must look like game UI). These cannot be combined cleanly with QSS.

---

## Decision

**Always prefer Qt's native property APIs over QSS.** Specifically:

| Goal | Use this, not QSS |
|---|---|
| Set widget colors / background | `QPalette` via `widget->setPalette()` |
| Set font family / size / weight | `QFont` + `widget->setFont()` |
| Custom widget drawing | Subclass `QWidget`, override `paintEvent`, use `QPainter` |
| Override platform drawing for a widget class | Subclass `QProxyStyle`, override draw methods |
| Widget size constraints | `setFixedSize()`, `setMinimumSize()`, `setMaximumSize()`, `setContentsMargins()` |
| Spacing and layout | `QLayout::setSpacing()`, `QLayout::setContentsMargins()` |
| Border / frame appearance | `QFrame::setFrameShape()`, `QFrame::setLineWidth()`, custom `paintEvent` |

QSS is permitted only for **rapid one-off prototyping** that will be replaced before the code is committed.

---

## Consequences

- Widget styling requires slightly more code than a one-liner QSS string. This is acceptable — the verbosity is explicit, the types are checked by the compiler, and the behavior is stable across Qt versions.
- `QPalette` covers the common cases (background color, text color, highlight color) fully. `QFont` covers typography. The native APIs are not limiting in practice.
- Custom drawing via `paintEvent` + `QPainter` is the correct technique for the overlay UI anyway (rendering game-themed elements over a transparent window) — so the project is already on this path.
- When upgrading Qt versions, styling code written against native APIs will either compile and work, or produce a compile error. QSS breakage is silent; native API breakage is loud.
