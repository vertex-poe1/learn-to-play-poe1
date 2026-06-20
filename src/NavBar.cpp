#include "NavBar.h"

#include <QMouseEvent>
#include <QPainter>

NavBar::NavBar(const QStringList &labels, QWidget *parent)
    : QWidget(parent), m_labels(labels)
{
}

void NavBar::setCurrentIndex(int index)
{
    if (index == m_current || index < 0 || index >= m_labels.size())
        return;
    m_current = index;
    update();
    emit currentChanged(index);
}

QSize NavBar::sizeHint() const
{
    QFont f = font();
    f.setPointSizeF(font().pointSizeF() * 2.0);
    return {0, QFontMetrics(f).height() + 28};
}

void NavBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    const int w = width();
    const int h = height();
    const int n = m_labels.size();
    if (n == 0) return;

    const int colW = w / n;
    const int separatorH = 3;
    const int underlineH = 6;

    p.fillRect(rect(), palette().window());

    // Bottom separator
    p.fillRect(0, h - separatorH, w, separatorH, palette().mid().color());

    QFont f = font();
    f.setPointSizeF(font().pointSizeF() * 2.0);

    for (int i = 0; i < n; ++i) {
        const int x = i * colW;
        const int cw = (i == n - 1) ? w - x : colW;
        const QRect cell(x, 0, cw, h - separatorH);
        const bool active = (i == m_current);

        f.setBold(active);
        p.setFont(f);
        p.setPen(active ? palette().windowText().color()
                        : palette().placeholderText().color());
        p.drawText(cell, Qt::AlignCenter, m_labels[i]);

        if (active) {
            p.fillRect(x, h - separatorH - underlineH, cw, underlineH,
                       palette().highlight().color());
        }
    }
}

void NavBar::mousePressEvent(QMouseEvent *event)
{
    const int n = m_labels.size();
    if (n == 0) return;
    const int col = qBound(0, static_cast<int>(event->position().x() * n / width()), n - 1);
    setCurrentIndex(col);
}
