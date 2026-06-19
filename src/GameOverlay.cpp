#include "GameOverlay.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>

GameOverlay::GameOverlay(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);

    m_infoPanel = new QLabel(QStringLiteral("Learn to Play PoE1"), this);
    m_infoPanel->setStyleSheet(
        "background: rgba(15, 10, 2, 210);"
        "color: #c8a84b;"
        "padding: 6px 10px;"
        "border-radius: 4px;"
        "border: 1px solid rgba(200, 168, 75, 0.45);");
    m_infoPanel->adjustSize();
}

void GameOverlay::updateGameRect(const QRect &physicalRect)
{
#ifdef Q_OS_WIN
    // Win32 GetWindowRect returns physical px; Qt setGeometry wants logical px.
    const QScreen *scr = QGuiApplication::primaryScreen();
    const qreal    dpr = scr ? scr->devicePixelRatio() : 1.0;
    setGeometry(QRect(
        qRound(physicalRect.x()      / dpr),
        qRound(physicalRect.y()      / dpr),
        qRound(physicalRect.width()  / dpr),
        qRound(physicalRect.height() / dpr)
    ));
#else
    setGeometry(physicalRect);
#endif
}

void GameOverlay::setGameVisible(bool found)
{
    setVisible(found);
}

void GameOverlay::paintEvent(QPaintEvent *)
{
    // Clear the entire surface to transparent so the game shows through.
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(rect(), Qt::transparent);
}

void GameOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    repositionPanels();
}

void GameOverlay::repositionPanels()
{
    if (!m_infoPanel)
        return;
    m_infoPanel->adjustSize();
    const int margin = 10;
    m_infoPanel->move(width() - m_infoPanel->width() - margin, margin);
    updateMask();
}

void GameOverlay::updateMask()
{
    // Only the panel regions intercept mouse events; everything else is click-through.
    QRegion mask;
    if (m_infoPanel)
        mask |= QRegion(m_infoPanel->geometry());
    setMask(mask);
}
