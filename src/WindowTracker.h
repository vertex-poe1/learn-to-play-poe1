#pragma once

#include <QRect>
#include <QString>

struct WindowState {
    bool    found{false};
    QRect   rect;
    QString installDir;
};

class WindowTracker {
public:
    virtual ~WindowTracker() = default;

    // Returns current state of the target process window.
    // executableName is the bare filename (e.g. "PathOfExile.exe").
    virtual WindowState poll(const QString &executableName) = 0;

    static WindowTracker *create();
};
