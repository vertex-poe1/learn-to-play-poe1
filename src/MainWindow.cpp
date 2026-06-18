#include "MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QIcon>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSystemTrayIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Learn to Play PoE1");
    setWindowIcon(QIcon(":/icons/vertex-icon.png"));
    resize(720, 480);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    setCentralWidget(m_log);

    setupTray();
    log("Application started.");
}

void MainWindow::setupTray()
{
    const QIcon icon(":/icons/vertex-icon.png");

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("Open", this, &MainWindow::showWindow);
    auto *settings = m_trayMenu->addAction("Settings");
    settings->setEnabled(false);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Exit", qApp, &QApplication::quit);

    m_tray = new QSystemTrayIcon(icon, this);
    m_tray->setContextMenu(m_trayMenu);
    m_tray->setToolTip("Learn to Play PoE1");
    connect(m_tray, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayActivated);
    m_tray->show();
}

void MainWindow::showWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        showWindow();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::log(const QString &message)
{
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(ts, message));
}
