#pragma once

#include <QWidget>

#ifdef _WIN32
class OverlayKeepalive;
#endif

class GameOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit GameOverlay(QWidget *parent = nullptr);
    ~GameOverlay() override;

signals:
    void showMainWindowRequested();

public:
    // Reposition and resize the overlay to cover the given game window rect (physical px on Windows).
    void updateGameRect(const QRect &physicalRect);

    // Show overlay when the game window is present, hide otherwise.
    void setGameVisible(bool found);

    void setLayoutVertical(bool vertical);
    void setHideoutVisible(bool visible);
    void setGuildVisible(bool visible);
    void setL2PVisible(bool visible);
    void setGameHwnd(quint64 hwnd);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void repositionPanels();
    void updateMask();

    QWidget *m_panelContainer{};
    QWidget *m_infoPanel{};
    QWidget *m_hideoutIcon{};
    QWidget *m_guildIcon{};

#ifdef _WIN32
    OverlayKeepalive *m_keepalive{};
#endif
};
