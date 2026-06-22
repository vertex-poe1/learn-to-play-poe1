#pragma once

#include "Database.h"
#include <QResizeEvent>
#include <QShowEvent>
#include <QStringList>
#include <QWidget>

struct LiveEvent;
class QueryService;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QTimer;
class QVBoxLayout;
class ScrollJumpButton;

class DmPage : public QWidget
{
    Q_OBJECT
public:
    explicit DmPage(QWidget *parent = nullptr);

    void setQueryService(QueryService *qs);
    void setShowGuildTags(bool show);
    void reload();

public slots:
    void onLiveWhisper(const LiveEvent &event);

protected:
    void showEvent(QShowEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onPlayerSelected(const QString &name);

private:
    void rebuild();
    void applyWhispers(const QList<Database::WhisperRecord> &whispers);
    void openFilterPanel();
    void refreshFilterPanel();
    void filterLeafSelected(const QString &name);
    void scrollToBottom();
    void updateScrollDownBtn();

    QueryService *m_queryService{};
    QLabel       *m_conversationLabel{};
    QPushButton  *m_filterBtn{};
    QString       m_filterPlayer;
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

    ScrollJumpButton              *m_scrollDownBtn{};
    QStackedWidget                *m_view{};
    QWidget                       *m_filterPanel{};
    QScrollArea                   *m_filterScroll{};
    QWidget                       *m_filterListWidget{};
    QVBoxLayout                   *m_filterListLayout{};
    QLabel                        *m_filterTitle{};
    QPushButton                   *m_backBtn{};
    QStringList                    m_filterPath;
    QList<Database::PartnerRecord> m_cachedPartners;
};
