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
    void setGearActive(bool active);

signals:
    void currentChanged(int index);
    void settingsClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

private:
    static constexpr int k_gearWidth = 48;
    QStringList m_labels;
    int  m_current{0};
    bool m_gearActive{false};
};
