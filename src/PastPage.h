#pragma once

#include "Database.h"
#include <QResizeEvent>
#include <QShowEvent>
#include <QWidget>

class LiveEvent;
class QScrollArea;
class QVBoxLayout;
class ScrollJumpButton;

class PastPage : public QWidget
{
    Q_OBJECT
public:
    explicit PastPage(Database *db, QWidget *parent = nullptr);
    void setDatabase(Database *db);
    void markDirty();

protected:
    void showEvent(QShowEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onLiveEvent(const LiveEvent &event);

private:
    void rebuild();
    void scrollToBottom();
    void updateScrollDownBtn();

    Database    *m_db{};
    QScrollArea *m_scroll{};
    QWidget     *m_content{};
    QVBoxLayout *m_contentLayout{};
    bool         m_dirty{true};
    int          m_limit{100};

    ScrollJumpButton *m_scrollDownBtn{};
};
