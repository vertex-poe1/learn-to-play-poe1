#include "core/DeferredTaskQueue.h"

#include <QGuiApplication>
#include <QCursor>
DeferredTaskQueue& DeferredTaskQueue::instance()
{
    static DeferredTaskQueue inst;
    return inst;
}

DeferredTaskQueue::DeferredTaskQueue(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &DeferredTaskQueue::processNextTask);
}

void DeferredTaskQueue::enqueue(const QString& id, int priority, std::function<void()> task)
{
    for (auto& t : m_tasks) {
        if (t.id == id) {
            t.priority = priority;
            t.work = std::move(task);
            if (!m_timer.isActive())
                m_timer.start();
            updateWaitCursor();
            return;
        }
    }
    m_tasks.append({id, priority, std::move(task)});
    if (!m_timer.isActive())
        m_timer.start();
    updateWaitCursor();
}

void DeferredTaskQueue::setPriority(const QString& id, int priority)
{
    for (auto& t : m_tasks) {
        if (t.id == id) {
            t.priority = priority;
            if (!m_timer.isActive())
                m_timer.start();
            updateWaitCursor();
            return;
        }
    }
}

void DeferredTaskQueue::cancel(const QString& id)
{
    m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
        [&id](const Task& t) { return t.id == id; }), m_tasks.end());
    
    if (m_tasks.isEmpty()) {
        m_timer.stop();
    }
    updateWaitCursor();
}

void DeferredTaskQueue::processNextTask()
{
    if (m_tasks.isEmpty())
        return;

    // Find highest priority task
    auto highestIt = m_tasks.begin();
    for (auto it = m_tasks.begin() + 1; it != m_tasks.end(); ++it) {
        if (it->priority > highestIt->priority) {
            highestIt = it;
        }
    }

    Task task = std::move(*highestIt);
    m_tasks.erase(highestIt);

    // Schedule next processing if there are remaining tasks
    if (!m_tasks.isEmpty()) {
        m_timer.start();
    }

    // Execute task
    if (task.work) {
        task.work();
    }
    
    updateWaitCursor();
}

void DeferredTaskQueue::updateWaitCursor()
{
    bool needsWaitCursor = false;
    for (const auto& t : m_tasks) {
        if (t.priority >= Immediate) {
            needsWaitCursor = true;
            break;
        }
    }

    if (needsWaitCursor && !m_waitCursorActive) {
        m_waitCursorActive = true;
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    } else if (!needsWaitCursor && m_waitCursorActive) {
        m_waitCursorActive = false;
        QGuiApplication::restoreOverrideCursor();
    }
}
