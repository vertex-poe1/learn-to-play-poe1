#include "DmPage.h"
#include "Database.h"
#include "LiveEvent.h"
#include "LiveEventBus.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDebug>
#include <QElapsedTimer>
#include <QFrame>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

// ---- DateSeparator ----------------------------------------------------------

class DateSeparator : public QWidget
{
public:
    explicit DateSeparator(const QString &date, QWidget *parent = nullptr)
        : QWidget(parent), m_date(date)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QFont f = font();
        f.setPointSizeF(f.pointSizeF() + 2);
        setFont(f);
    }

    QSize sizeHint() const override
    {
        return {0, fontMetrics().height() + 20};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const int w   = width();
        const int mid = height() / 2;

        const int textW = fontMetrics().horizontalAdvance(m_date) + 16;
        const int textX = (w - textW) / 2;

        p.setPen(palette().mid().color());
        p.drawLine(16, mid, textX - 6, mid);
        p.drawLine(textX + textW + 6, mid, w - 16, mid);

        p.setPen(palette().placeholderText().color());
        p.drawText(textX, 0, textW, height(), Qt::AlignCenter, m_date);
    }

private:
    QString m_date;
};

// ---- WhisperBubble ----------------------------------------------------------

class WhisperBubble : public QWidget
{
public:
    WhisperBubble(const QString &direction, const QString &playerName,
                  const QString &message,   const QString &time,
                  bool showName, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_direction(direction)
        , m_playerName(playerName)
        , m_message(message)
        , m_time(time)
        , m_showName(showName)
    {
        QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sp.setHeightForWidth(true);
        setSizePolicy(sp);
        QFont f = font();
        f.setPointSizeF(f.pointSizeF() + 2);
        setFont(f);
    }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int w) const override
    {
        const int bw = bubbleWidth(w);
        if (bw <= 0) return 48;
        const int tw = qMax(bw - 2 * kPad, 1);

        QFontMetrics fm(font());
        QFont boldF = font();
        boldF.setBold(true);

        int h = kPad;
        if (m_showName)
            h += QFontMetrics(boldF).height() + 4;
        h += fm.boundingRect(0, 0, tw, 10000, Qt::TextWordWrap, m_message).height();
        h += 4 + fm.height(); // gap + time row
        h += kPad;
        return h + 2 * kMargin;
    }

    QSize sizeHint() const override
    {
        return {200, heightForWidth(width() > 0 ? width() : 400)};
    }

protected:
    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        if (e->size().width() != e->oldSize().width())
            updateGeometry();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const bool out = (m_direction == "to");
        const int  w   = width();
        const int  bw  = bubbleWidth(w);
        const int  bx  = out ? w - kMargin - bw : kMargin;
        const int  tw  = qMax(bw - 2 * kPad, 1);

        QFontMetrics fm(font());
        QFont boldF = font();
        boldF.setBold(true);

        // Compute bubble height (same logic as heightForWidth)
        int bh = kPad;
        if (m_showName) bh += QFontMetrics(boldF).height() + 4;
        const QRect msgR = fm.boundingRect(0, 0, tw, 10000, Qt::TextWordWrap, m_message);
        bh += msgR.height() + 4 + fm.height() + kPad;

        // Background
        const QColor bg = out ? palette().highlight().color()
                               : palette().alternateBase().color();
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(bx, kMargin, bw, bh, kRadius, kRadius);

        const QColor fg  = out ? palette().highlightedText().color()
                                : palette().windowText().color();
        const QColor dim = out ? palette().highlightedText().color().darker(140)
                                : palette().placeholderText().color();

        int y = kMargin + kPad;

        // Name row
        if (m_showName) {
            const int nh = QFontMetrics(boldF).height();
            p.setFont(boldF);
            p.setPen(fg);
            const QString label = out ? (QString("→ ") + m_playerName) : m_playerName;
            p.drawText(bx + kPad, y, tw, nh, Qt::AlignLeft | Qt::AlignVCenter, label);
            y += nh + 4;
        }

        // Message
        p.setFont(font());
        p.setPen(fg);
        p.drawText(bx + kPad, y, tw, msgR.height(), Qt::TextWordWrap, m_message);
        y += msgR.height() + 4;

        // Timestamp (right-aligned, slightly smaller)
        QFont smallF = font();
        smallF.setPointSizeF(font().pointSizeF() * 0.82);
        p.setFont(smallF);
        p.setPen(dim);
        p.drawText(bx + kPad, y, tw, QFontMetrics(smallF).height(),
                   Qt::AlignRight | Qt::AlignVCenter, m_time);
    }

private:
    int bubbleWidth(int containerW) const
    {
        return qMin(static_cast<int>(containerW * kMaxRatio), containerW - 2 * kMargin);
    }

    static constexpr int    kMargin   = 8;
    static constexpr int    kPad      = 10;
    static constexpr double kMaxRatio = 0.65;
    static constexpr int    kRadius   = 8;

    QString m_direction;
    QString m_playerName;
    QString m_message;
    QString m_time;
    bool    m_showName;
};

// ---- BoundedComboBox --------------------------------------------------------

class BoundedComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;
    void showPopup() override
    {
        QWidget *win = window();
        if (win)
            view()->setMaximumHeight(win->height());
        QComboBox::showPopup();
        // Qt sizes the popup container independently of the view; clamp it too.
        if (win) {
            if (QWidget *container = view()->parentWidget()) {
                QRect geo    = container->geometry();
                QRect winGeo = win->geometry();
                if (geo.bottom() > winGeo.bottom())
                    geo.setBottom(winGeo.bottom());
                if (geo.top() < winGeo.top())
                    geo.setTop(winGeo.top());
                container->setGeometry(geo);
            }
        }
    }
};

// ---- DmPage ----------------------------------------------------------------

DmPage::DmPage(Database *db, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
{
    m_filter = new BoundedComboBox(this);
    m_filter->addItem("All conversations");

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 4, 0, 0);
    vbox->setSpacing(4);
    vbox->addWidget(m_filter);
    vbox->addWidget(m_scroll, 1);

    m_content = new QWidget;
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->addStretch(1);
    m_scroll->setWidget(m_content);

    connect(m_filter, &QComboBox::currentIndexChanged,
            this, &DmPage::onFilterChanged);

    m_liveRebuildTimer = new QTimer(this);
    m_liveRebuildTimer->setSingleShot(true);
    m_liveRebuildTimer->setInterval(300);
    connect(m_liveRebuildTimer, &QTimer::timeout, this, [this] {
        if (!isVisible()) { m_dirty = true; return; }
        populateFilter();
        rebuild();
        if (m_liveRebuildScrollToBottom)
            QTimer::singleShot(0, this, &DmPage::scrollToBottom);
    });

    connect(LiveEventBus::instance(), &LiveEventBus::eventFired,
            this, &DmPage::onLiveWhisper);
}

void DmPage::setDatabase(Database *db)
{
    m_db = db;
}

void DmPage::reload()
{
    m_dirty = false;
    m_limit = 100;
    populateFilter();
    rebuild();
    QTimer::singleShot(0, this, &DmPage::scrollToBottom);
}

void DmPage::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if (m_dirty && m_db)
        reload();
}

void DmPage::onLiveWhisper(const LiveEvent &event)
{
    if (event.type != LiveEventType::Whisper) return;

    if (!isVisible()) {
        m_dirty = true;
        return;
    }

    // Capture scroll position at the leading edge of each burst so we know
    // whether to auto-scroll after the debounced rebuild fires.
    if (!m_liveRebuildTimer->isActive())
        m_liveRebuildScrollToBottom = m_scroll->verticalScrollBar()->value()
                                      >= m_scroll->verticalScrollBar()->maximum() - 4;

    m_liveRebuildTimer->start(); // restarts the 300 ms window on every event
}

void DmPage::onFilterChanged(int /*index*/)
{
    m_limit = 100;
    rebuild();
    QTimer::singleShot(0, this, &DmPage::scrollToBottom);
}

void DmPage::rebuild()
{
    if (!m_db) return;

    QElapsedTimer t; t.start();

    const QString filterPlayer = m_filter->currentIndex() > 0
                                     ? m_filter->currentText()
                                     : QString{};
    const bool showNames = filterPlayer.isEmpty();

    const QList<Database::WhisperRecord> whispers = m_db->fetchWhispers(filterPlayer, m_limit);
    qDebug() << "[DmPage] fetchWhispers returned" << whispers.size() << "rows in" << t.elapsed() << "ms";

    // Build content widget fully before handing it to the scroll area so that
    // the layout engine only performs one pass instead of N passes (one per addWidget).
    auto *content       = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(2);
    contentLayout->addStretch(1);

    if (whispers.size() == m_limit) {
        auto *btn = new QPushButton(
            QStringLiteral("Load previous 50 messages"), content);
        btn->setFlat(true);
        connect(btn, &QPushButton::clicked, this, [this] {
            m_limit += 50;
            rebuild();
        });
        contentLayout->addWidget(btn);
    }

    QString lastDate;
    for (const auto &w : whispers) {
        const QString date = w.occurredAt.left(10);
        if (date != lastDate) {
            lastDate = date;
            contentLayout->addWidget(new DateSeparator(date, content));
        }
        const QString time = w.occurredAt.mid(11, 5);
        contentLayout->addWidget(
            new WhisperBubble(w.direction, w.playerName, w.message,
                              time, showNames, content));
    }

    qDebug() << "[DmPage] widgets built in" << t.elapsed() << "ms";
    // Replace live references and hand the fully-built widget to the scroll area.
    delete m_content;
    m_content       = content;
    m_contentLayout = contentLayout;
    m_scroll->setWidget(m_content);
    qDebug() << "[DmPage] rebuild done in" << t.elapsed() << "ms";
}

void DmPage::populateFilter()
{
    if (!m_db) return;

    const QString current = m_filter->currentIndex() > 0
                                ? m_filter->currentText()
                                : QString{};

    m_filter->blockSignals(true);
    m_filter->clear();
    m_filter->addItem("All conversations");
    for (const QString &name : m_db->fetchWhisperPartners())
        m_filter->addItem(name);

    if (!current.isEmpty()) {
        const int idx = m_filter->findText(current);
        m_filter->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    m_filter->blockSignals(false);
}

void DmPage::scrollToBottom()
{
    m_scroll->verticalScrollBar()->setValue(
        m_scroll->verticalScrollBar()->maximum());
}
