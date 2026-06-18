#include "GameOverlay.h"

#include <QLabel>
#include <QPainter>
#include <QResizeEvent>

GameOverlay::GameOverlay(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);

    m_infoPanel = new QLabel(QStringLiteral("Learn to Play PoE1"), this);
    m_infoPanel->setStyleSheet(
        "background: rgba(0, 0, 0, 180);"
        "color: white;"
        "padding: 6px 10px;"
        "border-radius: 4px;");
    m_infoPanel->adjustSize();
}

void GameOverlay::updateGameRect(const QRect &gameRect)
{
    // TODO Phase 5: convert Win32 physical px → Qt logical px via QScreen::devicePixelRatio
    setGeometry(gameRect);
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
