#pragma once

#include <QShowEvent>
#include <QWidget>

class Database;
class LiveEvent;
class QPushButton;
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
    void onPlayerSelected(const QString &name);

private:
    void rebuild();
    void showFilterMenu();
    void scrollToBottom();

    Database    *m_db{};
    QPushButton *m_filterBtn{};
    QString      m_filterPlayer;
    QScrollArea *m_scroll{};
    QWidget     *m_content{};
    QVBoxLayout *m_contentLayout{};
    QTimer      *m_liveRebuildTimer{};
    bool         m_liveRebuildScrollToBottom{false};
    bool         m_dirty{true};
    int          m_limit{100};
};
