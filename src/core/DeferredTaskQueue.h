#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QList>
#include <functional>

class DeferredTaskQueue : public QObject
{
    Q_OBJECT

public:
    enum Priority {
        Low = 0,
        Normal = 1,
        High = 2,
        Immediate = 10
    };

    static DeferredTaskQueue& instance();

    void enqueue(const QString& id, int priority, std::function<void()> task);
    void setPriority(const QString& id, int priority);
    void cancel(const QString& id);

private slots:
    void processNextTask();

private:
    explicit DeferredTaskQueue(QObject *parent = nullptr);
    ~DeferredTaskQueue() override = default;

    struct Task {
        QString id;
        int priority;
        std::function<void()> work;
    };

    void updateWaitCursor();

    QList<Task> m_tasks;
    QTimer m_timer;
    bool m_waitCursorActive = false;
};
