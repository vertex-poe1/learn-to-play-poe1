#pragma once

#include <QColor>
#include <QFrame>

struct NotificationStyle {
    QColor background{45, 45, 45};
    QColor border{80, 80, 80};
    QColor textColor{Qt::white};
    QColor bodyColor{180, 180, 180};
    QColor timestampColor{140, 140, 140};
    int    borderRadius{6};
    int    borderWidth{1};
};

class NotificationWidget : public QFrame
{
    Q_OBJECT
public:
    explicit NotificationWidget(const QString &title, const QString &tag,
                                const QString &message, const QString &timestamp,
                                const NotificationStyle &style = {},
                                QWidget *parent = nullptr);
};
