#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class QPlainTextEdit;
class QAction;
class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void log(const QString &message);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void showWindow();
    void setupTray();

    QPlainTextEdit *m_log{};
    QSystemTrayIcon *m_tray{};
    QMenu *m_trayMenu{};
};
