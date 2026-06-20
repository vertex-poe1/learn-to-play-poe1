#pragma once

#include <QStringList>
#include <QWidget>

class NavBar : public QWidget
{
    Q_OBJECT

public:
    explicit NavBar(const QStringList &labels, QWidget *parent = nullptr);

    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

private:
    QStringList m_labels;
    int m_current{0};
};
