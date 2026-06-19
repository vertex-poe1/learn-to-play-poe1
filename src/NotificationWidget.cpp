#include "NotificationWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace {

// Draws its text inside a pill (fully-rounded rectangle) using QPainter so
// the shape is always correct regardless of Qt's stylesheet border-radius quirks.
class TagLabel : public QLabel
{
public:
    explicit TagLabel(const QString &text, const QColor &textColor, QWidget *parent = nullptr)
        : QLabel(text, parent), m_textColor(textColor)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setContentsMargins(5, 0, 5, 0); // horizontal inset only, for legibility
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int r = height() / 2;
        const QRect border = rect().adjusted(0, 0, -1, -1);

        p.setPen(QPen(QColor(255, 255, 255, 76), 1)); // rgba(255,255,255,0.3)
        p.setBrush(QColor(255, 255, 255, 13));         // rgba(255,255,255,0.05)
        p.drawRoundedRect(border, r, r);

        const QFontMetrics fm(font());
        const int baseline = (height() + fm.ascent() - fm.descent()) / 2 - 1;
        p.setPen(m_textColor);
        p.drawText(contentsMargins().left(), baseline, text());
    }

private:
    QColor m_textColor;
};

QWidget *buildSegmentedRow(const QString &text, const QColor &color, int fontPt, QWidget *parent)
{
    auto *row    = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    const QString plainStyle = QStringLiteral(
        "QLabel { border: none; background: transparent; color: %1; font-size: %2pt; }")
        .arg(color.name()).arg(fontPt);
    const QString badgeStyle = QStringLiteral(
        "QLabel { background: transparent; color: %1;"
        " border: 1px solid rgba(255,255,255,0.3);"
        " border-radius: 4px; padding: 1px 0;"
        " font-size: %2pt; font-family: monospace; }")
        .arg(color.name()).arg(fontPt - 2);

    static const QRegularExpression re("\\{([^}]*)\\}");
    int lastEnd = 0;
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        if (match.capturedStart() > lastEnd) {
            auto *lbl = new QLabel(text.mid(lastEnd, match.capturedStart() - lastEnd), row);
            lbl->setStyleSheet(plainStyle);
            layout->addWidget(lbl);
        }
        auto *badge = new QLabel(match.captured(1), row);
        badge->setStyleSheet(badgeStyle);
        layout->addWidget(badge);
        lastEnd = match.capturedEnd();
    }
    if (lastEnd < text.length()) {
        auto *lbl = new QLabel(text.mid(lastEnd), row);
        lbl->setStyleSheet(plainStyle);
        layout->addWidget(lbl);
    }
    layout->addStretch();
    return row;
}

} // namespace

NotificationWidget::NotificationWidget(const QString &title, const QString &tag,
                                       const QString &message, const QString &timestamp,
                                       const NotificationStyle &style, QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet(QStringLiteral(
        "NotificationWidget {"
        "  background-color: %1;"
        "  border: %2px solid %3;"
        "  border-radius: %4px;"
        "}"
    ).arg(style.background.name())
     .arg(style.borderWidth)
     .arg(style.border.name())
     .arg(style.borderRadius));

    auto *tsLabel = new QLabel(timestamp, this);
    tsLabel->setStyleSheet(QStringLiteral(
        "QLabel { border: none; background: transparent; color: %1; font-size: 11pt; }")
        .arg(style.timestampColor.name()));
    tsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    tsLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(10, 8, 10, 8);
    outer->setSpacing(8);

    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(8);

    if (!title.isEmpty()) {
        auto *left       = new QWidget(this);
        auto *leftLayout = new QHBoxLayout(left);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(6);

        auto *titleLabel = new QLabel(title, left);
        titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { border: none; background: transparent; color: %1;"
            " font-weight: bold; font-size: 15pt; }")
            .arg(style.textColor.name()));
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        leftLayout->addWidget(titleLabel, 0, Qt::AlignTop);

        if (!tag.isEmpty()) {
            auto *tagLabel = new TagLabel(tag, style.timestampColor, left);
            tagLabel->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 8pt; }"));
            leftLayout->addWidget(tagLabel, 0, Qt::AlignTop);
        }

        leftLayout->addStretch();
        topRow->addWidget(left, 1);
    } else {
        topRow->addWidget(buildSegmentedRow(message, style.textColor, 12, this), 1);
    }

    topRow->addWidget(tsLabel, 0);
    outer->addLayout(topRow);

    if (!title.isEmpty())
        outer->addWidget(buildSegmentedRow(message, style.bodyColor, 10, this));
}
