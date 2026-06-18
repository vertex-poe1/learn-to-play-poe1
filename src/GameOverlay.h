#pragma once

#include <QWidget>

class QLabel;

class GameOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit GameOverlay(QWidget *parent = nullptr);

    // Reposition and resize the overlay to cover the given screen rect.
    void updateGameRect(const QRect &gameRect);

    // Show overlay when the game window is present, hide otherwise.
    void setGameVisible(bool found);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void repositionPanels();
    void updateMask();

    QLabel *m_infoPanel{};
};
