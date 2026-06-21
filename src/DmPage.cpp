#include "DmPage.h"
#include "Database.h"
#include "LiveEvent.h"
#include "LiveEventBus.h"

#include <functional>
#include <QDate>
#include <QDebug>
#include <QElapsedTimer>
#include <QFrame>
#include <QLocale>
#include <QMap>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
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

// ---- Filter menu helpers ----------------------------------------------------

static constexpr int kAlphaThreshold = 100;

// Adds names to parent as a flat list (<=kAlphaThreshold and !alwaysAlpha) or
// as sym + A–Z submenus. Empty letter submenus are shown grayed out.
static void addNamesToMenu(QMenu *parent, const QStringList &names,
                           bool alwaysAlpha,
                           const std::function<void(const QString &)> &onSelect)
{
    if (!alwaysAlpha && names.size() <= kAlphaThreshold) {
        for (const QString &name : names)
            parent->addAction(name, [name, onSelect] { onSelect(name); });
        return;
    }

    QStringList syms;
    QMap<QChar, QStringList> byLetter;
    for (const QString &name : names) {
        if (name.isEmpty()) continue;
        if (!name[0].isLetter())
            syms << name;
        else
            byLetter[name[0].toUpper()] << name;
    }

    // sym submenu
    QMenu *symMenu = parent->addMenu("sym");
    if (syms.isEmpty()) {
        symMenu->menuAction()->setEnabled(false);
    } else {
        for (const QString &name : syms)
            symMenu->addAction(name, [name, onSelect] { onSelect(name); });
    }

    // A–Z submenus
    for (char c = 'A'; c <= 'Z'; ++c) {
        const QChar letter(c);
        const QStringList lnames = byLetter.value(letter);
        QMenu *letMenu = parent->addMenu(QString(letter));
        if (lnames.isEmpty()) {
            letMenu->menuAction()->setEnabled(false);
        } else {
            for (const QString &name : lnames)
                letMenu->addAction(name, [name, onSelect] { onSelect(name); });
        }
    }
}

// ---- DmPage ----------------------------------------------------------------

DmPage::DmPage(Database *db, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
{
    m_filterBtn = new QPushButton("All conversations", this);
    connect(m_filterBtn, &QPushButton::clicked, this, &DmPage::showFilterMenu);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 4, 0, 0);
    vbox->setSpacing(4);
    vbox->addWidget(m_filterBtn);
    vbox->addWidget(m_scroll, 1);

    m_content = new QWidget;
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->addStretch(1);
    m_scroll->setWidget(m_content);

    m_liveRebuildTimer = new QTimer(this);
    m_liveRebuildTimer->setSingleShot(true);
    m_liveRebuildTimer->setInterval(300);
    connect(m_liveRebuildTimer, &QTimer::timeout, this, [this] {
        if (!isVisible()) { m_dirty = true; return; }
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

void DmPage::onPlayerSelected(const QString &name)
{
    if (m_filterPlayer == name) return;
    m_filterPlayer = name;
    m_filterBtn->setText(name.isEmpty() ? "All conversations" : name);
    m_limit = 100;
    rebuild();
    QTimer::singleShot(0, this, &DmPage::scrollToBottom);
}

void DmPage::showFilterMenu()
{
    if (!m_db) return;

    const auto partners = m_db->fetchWhisperPartnersWithDates();
    const QDate today   = QDate::currentDate();

    QMenu menu(this);

    // "All conversations" resets the filter
    QAction *allAct = menu.addAction("All conversations");
    connect(allAct, &QAction::triggered, this, [this] { onPlayerSelected({}); });

    menu.addSeparator();

    // ---- Time bucket definitions (ISO date strings for fast string comparison) ----
    const QString todayStr  = today.toString(Qt::ISODate);
    const QString yestStr   = today.addDays(-1).toString(Qt::ISODate);
    const int     dow       = today.dayOfWeek();           // 1=Mon..7=Sun
    const QDate   sowDate   = today.addDays(1 - dow);      // Monday of this week
    const QString sowStr    = sowDate.toString(Qt::ISODate);
    const QString slwStr    = sowDate.addDays(-7).toString(Qt::ISODate);
    const QString elwStr    = sowDate.addDays(-1).toString(Qt::ISODate);
    const QString somStr    = QDate(today.year(), today.month(), 1).toString(Qt::ISODate);

    struct Bucket { QString label; QString from, to; };
    QList<Bucket> buckets;
    buckets << Bucket{"Today",      todayStr, todayStr};
    buckets << Bucket{"Yesterday",  yestStr,  yestStr};
    buckets << Bucket{"This Week",  sowStr,   todayStr};
    buckets << Bucket{"Last Week",  slwStr,   elwStr};
    buckets << Bucket{"This Month", somStr,   todayStr};

    // Previous months of this year
    const QLocale locale;
    for (int m = today.month() - 1; m >= 1; --m) {
        const QDate first = QDate(today.year(), m, 1);
        const QDate last  = first.addMonths(1).addDays(-1);
        buckets << Bucket{locale.standaloneMonthName(m),
                          first.toString(Qt::ISODate),
                          last.toString(Qt::ISODate)};
    }

    // Previous years present in the data
    QSet<int> years;
    for (const auto &p : partners)
        for (const QString &d : p.dates)
            years.insert(d.left(4).toInt());
    years.remove(today.year());

    QList<int> sortedYears = years.values();
    std::sort(sortedYears.begin(), sortedYears.end(), std::greater<int>());
    for (int y : sortedYears) {
        buckets << Bucket{QString::number(y),
                          QStringLiteral("%1-01-01").arg(y),
                          QStringLiteral("%1-12-31").arg(y)};
    }

    // ---- Add a submenu for each non-empty time bucket ----
    for (const Bucket &b : buckets) {
        QStringList names;
        for (const auto &p : partners) {
            for (const QString &d : p.dates) {
                if (d >= b.from && d <= b.to) {
                    names << p.name;
                    break;
                }
            }
        }
        if (names.isEmpty()) continue;

        names.sort(Qt::CaseInsensitive);
        QMenu *sub = menu.addMenu(b.label);
        addNamesToMenu(sub, names, false,
                       [this](const QString &n) { onPlayerSelected(n); });
    }

    menu.addSeparator();

    // ---- "All" submenu: always uses sym + A–Z structure ----
    {
        QStringList allNames;
        allNames.reserve(partners.size());
        for (const auto &p : partners)
            allNames << p.name;
        allNames.sort(Qt::CaseInsensitive);

        QMenu *allMenu = menu.addMenu("All");
        addNamesToMenu(allMenu, allNames, true,
                       [this](const QString &n) { onPlayerSelected(n); });
    }

    menu.exec(m_filterBtn->mapToGlobal(QPoint(0, m_filterBtn->height())));
}

void DmPage::rebuild()
{
    if (!m_db) return;

    QElapsedTimer t; t.start();

    const bool showNames = m_filterPlayer.isEmpty();

    const QList<Database::WhisperRecord> whispers = m_db->fetchWhispers(m_filterPlayer, m_limit);
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

void DmPage::scrollToBottom()
{
    m_scroll->verticalScrollBar()->setValue(
        m_scroll->verticalScrollBar()->maximum());
}
