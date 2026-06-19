#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Learn to Play PoE1");
    app.setApplicationVersion("0.1.0");
    app.setQuitOnLastWindowClosed(false);

    app.setStyleSheet(R"(
        QMainWindow, QDialog {
            background-color: #1e1e1e;
        }
        QMenuBar {
            background-color: #232323;
            color: #d0d0d0;
            border-bottom: 1px solid #333;
        }
        QMenuBar::item:selected {
            background-color: #323232;
            color: #c8a84b;
        }
        QMenu {
            background-color: #2a2a2a;
            color: #d0d0d0;
            border: 1px solid #444;
        }
        QMenu::item:selected {
            background-color: #363636;
            color: #c8a84b;
        }
        QMenu::separator {
            background-color: #404040;
            height: 1px;
            margin: 3px 0;
        }
        QCheckBox {
            color: #c8c8c8;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border: 1px solid #585858;
            border-radius: 3px;
            background-color: #2a2a2a;
        }
        QCheckBox::indicator:checked {
            background-color: #c8a84b;
            border-color: #c8a84b;
        }
        QCheckBox::indicator:disabled {
            background-color: #2e2e2e;
            border-color: #3a3a3a;
        }
        QLabel {
            background: transparent;
            color: #c0c0c0;
        }
        QLineEdit {
            background-color: #282828;
            color: #e0e0e0;
            border: 1px solid #525252;
            border-radius: 3px;
            padding: 3px 6px;
            selection-background-color: #c8a84b;
            selection-color: #1a1a1a;
        }
        QLineEdit:focus {
            border-color: #c8a84b;
        }
        QLineEdit:placeholder {
            color: #606060;
        }
        QPushButton {
            background-color: #2e2e2e;
            color: #d0d0d0;
            border: 1px solid #525252;
            border-radius: 4px;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: #383838;
            border-color: #c8a84b;
            color: #e8e8e8;
        }
        QPushButton:pressed {
            background-color: #242424;
        }
        QPushButton:disabled {
            color: #505050;
            border-color: #383838;
        }
        QListWidget {
            background-color: #242424;
            color: #c8c8c8;
            border: 1px solid #424242;
            border-radius: 3px;
            outline: none;
        }
        QListWidget::item {
            padding: 2px 4px;
        }
        QListWidget::item:selected {
            background-color: #363636;
            color: #c8a84b;
        }
        QListWidget::item:hover {
            background-color: #2d2d2d;
        }
        QScrollBar:vertical {
            background-color: #222222;
            width: 8px;
            margin: 0;
            border: none;
        }
        QScrollBar::handle:vertical {
            background-color: #484848;
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #c8a84b;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background-color: #222222;
            height: 8px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background-color: #484848;
            border-radius: 4px;
            min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #c8a84b;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
    )");

    MainWindow window;
    if (!window.startMinimized())
        window.show();

    return app.exec();
}
