#pragma once

#include <QShowEvent>
#include <QWidget>

class Database;
class LiveEvent;
class QComboBox;
class QScrollArea;
class QVBoxLayout;

class DmPage : public QWidget
{
    Q_OBJECT
public:
    explicit DmPage(Database *db, QWidget *parent = nullptr);

    void setDatabase(Database *db);
    void reload();

public slots:
    void onLiveWhisper(const LiveEvent &event);

protected:
    void showEvent(QShowEvent *e) override;

private slots:
    void onFilterChanged(int index);

private:
    void rebuild();
    void populateFilter();
    void scrollToBottom();

    Database    *m_db{};
    QComboBox   *m_filter{};
    QScrollArea *m_scroll{};
    QWidget     *m_content{};
    QVBoxLayout *m_contentLayout{};
    QTimer      *m_liveRebuildTimer{};
    bool         m_liveRebuildScrollToBottom{false};
    bool         m_dirty{true};
    int          m_limit{100};
};
