#pragma once

#include "Database.h"
#include <QResizeEvent>
#include <QSet>
#include <QShowEvent>
#include <QStringList>
#include <QWidget>

class LiveEvent;
class QueryService;
class QCheckBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QTimer;
class QVBoxLayout;
class ScrollJumpButton;

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);

    void setQueryService(QueryService *qs);
    void setShowGuildTags(bool show);
    void reload();

public slots:
    void onLiveChat(const LiveEvent &event);

protected:
    void showEvent(QShowEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void rebuild();
    void applyChats(const QList<Database::ChatRecord> &records);
    void openFilterPanel();
    void refreshFilterPanel();
    void scrollToBottom();
    void updateScrollDownBtn();
    QSet<QChar> activeChannels() const;
    void updateFilterLabel();

    QueryService *m_queryService{};
    QScrollArea  *m_scroll{};
    QWidget      *m_content{};
    QVBoxLayout  *m_contentLayout{};
    QTimer       *m_liveRebuildTimer{};
    bool          m_liveRebuildScrollToBottom{false};
    bool          m_dirty{true};
    bool          m_rebuildInFlight{false};
    bool          m_showGuildTags{true};
    int           m_limit{100};
    int           m_scrollRestorePrevMax{-1};
    int           m_scrollRestorePrevValue{0};

    QString m_fromDate;
    QString m_toDate;

    QCheckBox   *m_cbLocal{};
    QCheckBox   *m_cbGlobal{};
    QCheckBox   *m_cbParty{};
    QCheckBox   *m_cbDm{};
    QCheckBox   *m_cbTrade{};
    QCheckBox   *m_cbGuild{};
    QPushButton *m_filterBtn{};

    ScrollJumpButton *m_scrollDownBtn{};
    QStackedWidget   *m_view{};
    QWidget        *m_filterPanel{};
    QScrollArea    *m_filterScroll{};
    QLabel         *m_filterTitle{};
    QPushButton    *m_backBtn{};
    QStringList     m_filterPath;
    QStringList     m_cachedDates;
};
