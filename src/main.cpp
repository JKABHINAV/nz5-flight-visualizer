#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setStyleSheet(R"(
        QMainWindow { background-color: #0f172a; }
        QPushButton {
            padding: 9px 18px; border-radius: 6px; font-weight: bold;
            font-size: 13px; color: #ffffff; border: none;
        }
        QPushButton#btnLoad { background-color: #0284c7; }
        QPushButton#btnLoad:hover { background-color: #0369a1; }
        QPushButton#btnSort { background-color: #7c3aed; }
        QPushButton#btnSort:hover { background-color: #6d28d9; }
        QPushButton#btnGeo  { background-color: #059669; }
        QPushButton#btnGeo:hover  { background-color: #047857; }
        QPushButton#btnBin  { background-color: #d97706; }
        QPushButton#btnBin:hover  { background-color: #b45309; }
        QTableView {
            background-color: #1e293b; alternate-background-color: #263349;
            color: #e2e8f0; gridline-color: #334155; border: 1px solid #334155;
            border-radius: 6px;
        }
        QHeaderView::section {
            background-color: #0f172a; color: #38bdf8; font-weight: bold;
            padding: 6px; border: 1px solid #334155;
        }
        QStatusBar { background-color: #0b1120; color: #94a3b8; font-size: 12px; }
    )");

    MainWindow w("C:/Users/samsung/Desktop/World_10M.mbtiles");
    w.show();
    return app.exec();
}
