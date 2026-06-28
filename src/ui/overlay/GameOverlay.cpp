#include "ui/overlay/GameOverlay.h"
#include "ui/Theme.h"

#ifdef _WIN32
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0600
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform/OverlayKeepalive.h"
#endif

#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QScreen>
#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QThread>

namespace {

class InfoPanel : public QWidget
{
public:
    explicit InfoPanel(const QString &text, QWidget *parent = nullptr)
        : QWidget(parent), m_text(text)
    {
        setContentsMargins(12, 6, 9, 6);
        QFont f = font();
        f.setPointSizeF(Theme::fontLg);
        f.setItalic(true);
        f.setBold(true);
        f.setStyleHint(QFont::Serif);
        f.setFamilies({"Palatino Linotype", "Book Antiqua", "Palatino", "serif"});
        f.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
        setFont(f);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void setOnClick(std::function<void()> cb)
    {
        m_onClick = std::move(cb);
        setCursor(Qt::PointingHandCursor);
    }

    QSize sizeHint() const override
    {
        const QFontMetrics fm(font());
        const QRect br = fm.boundingRect(m_text);
        const QMargins m = contentsMargins();
        return {br.width() + m.left() + m.right(),
                br.height() + m.top() + m.bottom()};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor border = Theme::accent;
        border.setAlpha(115);  // 0.45 * 255

        p.setPen(QPen(border, 1));
        p.setBrush(QColor(15, 10, 2, 150));
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);

        p.setPen(Theme::accent);
        p.setFont(font());
        p.drawText(contentsRect(), Qt::AlignCenter, m_text);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_onClick) {
            m_onClick();
        }
    }

private:
    QString m_text;
    std::function<void()> m_onClick;
};

class ClickableIcon : public QWidget
{
public:
    explicit ClickableIcon(const QString &svgPath, const QString &command, const QColor &bgColor, const QColor &borderColor, QWidget *parent = nullptr)
        : QWidget(parent), m_pixmap(svgPath), m_command(command), m_bgColor(bgColor), m_borderColor(borderColor)
    {
        setFixedSize(36, 36);
        setCursor(Qt::PointingHandCursor);
    }

    void setGameHwnd(quint64 hwnd) { m_gameHwnd = hwnd; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.setPen(QPen(m_borderColor, 1));
        p.setBrush(m_bgColor);
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);

        if (!m_pixmap.isNull()) {
            QPixmap scaled = m_pixmap.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int dx = (width() - scaled.width()) / 2;
            int dy = (height() - scaled.height()) / 2;
            p.drawPixmap(dx, dy, scaled);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
#ifdef _WIN32
            if (m_gameHwnd != 0) {
                HWND hwnd = reinterpret_cast<HWND>(m_gameHwnd);
                if (GetForegroundWindow() != hwnd) {
                    SetForegroundWindow(hwnd);
                    QThread::msleep(50); // allow window to activate
                }
            }

            auto sendKey = [](WORD vk) {
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = vk;
                SendInput(1, &input, sizeof(INPUT));
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            };

            // Enter
            sendKey(VK_RETURN);
            QThread::msleep(20);

            // command
            for (QChar c : m_command) {
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = c.unicode();
                input.ki.dwFlags = KEYEVENTF_UNICODE;
                SendInput(1, &input, sizeof(INPUT));
                input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }

            QThread::msleep(20);
            // Enter
            sendKey(VK_RETURN);
#endif
        }
        QWidget::mousePressEvent(event);
    }

private:
    QPixmap m_pixmap;
    QString m_command;
    QColor m_bgColor;
    QColor m_borderColor;
    quint64 m_gameHwnd{0};
};

} // namespace

GameOverlay::GameOverlay(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);

    m_panelContainer = new QWidget(this);
    auto *layout = new QBoxLayout(QBoxLayout::TopToBottom, m_panelContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    m_infoPanel = new InfoPanel(QStringLiteral("l2p"), m_panelContainer);
    static_cast<InfoPanel*>(m_infoPanel)->setOnClick([this]() { emit showMainWindowRequested(); });
    layout->addWidget(m_infoPanel);

    QColor blueBorder = QColor(173, 216, 230);
    blueBorder.setAlpha(200);
    m_hideoutIcon = new ClickableIcon(QStringLiteral(":/icons/fleur-de-lis.svg"), QStringLiteral("/hideout"), QColor(30, 45, 65, 150), blueBorder, m_panelContainer);
    layout->addWidget(m_hideoutIcon, 0, Qt::AlignHCenter);

    QColor accentBorder = Theme::accent;
    accentBorder.setAlpha(115);
    m_guildIcon = new ClickableIcon(QStringLiteral(":/icons/fleur-de-lis-shield.svg"), QStringLiteral("/guild"), QColor(15, 10, 2, 150), accentBorder, m_panelContainer);
    layout->addWidget(m_guildIcon, 0, Qt::AlignHCenter);

    m_panelContainer->adjustSize();

#ifdef _WIN32
    // Calling winId() forces native HWND creation so we can read/set exstyles now.
    const auto hwnd = reinterpret_cast<HWND>(winId());
    const LONG ex   = GetWindowLong(hwnd, GWL_EXSTYLE);
    // Remove WS_EX_TRANSPARENT so mouse clicks can reach the overlay, but keep WS_EX_NOACTIVATE
    // to avoid stealing focus from the game.
    SetWindowLong(hwnd, GWL_EXSTYLE, (ex | WS_EX_LAYERED | WS_EX_NOACTIVATE) & ~WS_EX_TRANSPARENT);
    m_keepalive = new OverlayKeepalive(hwnd);
#endif
}

GameOverlay::~GameOverlay()
{
#ifdef _WIN32
    delete m_keepalive;
#endif
}

void GameOverlay::updateGameRect(const QRect &physicalRect)
{
#ifdef _WIN32
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

void GameOverlay::setLayoutVertical(bool vertical)
{
    if (auto *box = qobject_cast<QBoxLayout*>(m_panelContainer->layout())) {
        box->setDirection(vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    }
    repositionPanels();
}

void GameOverlay::setHideoutVisible(bool visible)
{
    m_hideoutIcon->setVisible(visible);
    repositionPanels();
}

void GameOverlay::setGuildVisible(bool visible)
{
    m_guildIcon->setVisible(visible);
    repositionPanels();
}

void GameOverlay::setGameHwnd(quint64 hwnd)
{
    if (m_hideoutIcon)
        static_cast<ClickableIcon*>(m_hideoutIcon)->setGameHwnd(hwnd);
    if (m_guildIcon)
        static_cast<ClickableIcon*>(m_guildIcon)->setGameHwnd(hwnd);
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
    if (!m_panelContainer)
        return;
    m_panelContainer->adjustSize();
    const int margin = 10;
    m_panelContainer->move(margin, margin);
    updateMask();
}

void GameOverlay::updateMask()
{
    // Only the panel regions intercept mouse events; everything else is click-through.
    QRegion mask;
    if (m_panelContainer)
        mask |= QRegion(m_panelContainer->geometry());
    setMask(mask);
}
