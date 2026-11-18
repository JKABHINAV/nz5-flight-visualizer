#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <QTableView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QStatusBar>
#include <QHeaderView>
#include "FlightTableModel.h"
#include "Map2DWidget.h"
#include "Globe3DWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& mbtilesPath, QWidget* parent = nullptr)
        : QMainWindow(parent) {
        setWindowTitle("NZ5 Flight Data Visualizer (2D / 3D Synchronized)");
        resize(1400, 820);

        auto* central = new QWidget(this);
        auto* rootLayout = new QVBoxLayout(central);
        rootLayout->setContentsMargins(14, 14, 14, 14);
        rootLayout->setSpacing(12);

        auto* btnLayout = new QHBoxLayout();
        auto* btnLoad = new QPushButton("Load CSV");
        auto* btnSort = new QPushButton("Sort");
        auto* btnGeo = new QPushButton("Write Geojson");
        auto* btnBin = new QPushButton("Write Binary");

        btnLoad->setObjectName("btnLoad");
        btnSort->setObjectName("btnSort");
        btnGeo->setObjectName("btnGeo");
        btnBin->setObjectName("btnBin");

        btnLayout->addWidget(btnLoad);
        btnLayout->addWidget(btnSort);
        btnLayout->addWidget(btnGeo);
        btnLayout->addWidget(btnBin);
        btnLayout->addStretch();
        rootLayout->addLayout(btnLayout);

        auto* splitter = new QSplitter(Qt::Horizontal, central);
        m_table = new QTableView(splitter);
        m_model = new FlightTableModel(this);
        m_table->setModel(m_model);
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->setAlternatingRowColors(true);

        m_map2d = new Map2DWidget(mbtilesPath, splitter);
        m_globe3d = new Globe3DWidget(mbtilesPath, splitter);

        splitter->addWidget(m_table);
        splitter->addWidget(m_map2d);
        splitter->addWidget(m_globe3d);
        splitter->setSizes({440, 480, 480});
        rootLayout->addWidget(splitter, 1);

        setCentralWidget(central);
        statusBar()->showMessage("Ready — Click 'Load CSV' to begin.");

        connect(btnLoad, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this, "Open Flight CSV", "", "CSV Files (*.csv)");
            if (path.isEmpty()) return;
            auto recs = FlightDataManager::loadCSV(path);
            m_model->setRecords(recs);
            m_map2d->setRecords(recs);
            m_globe3d->setRecords(recs);
            statusBar()->showMessage(QString("Loaded %1 flight records.").arg(recs.size()));
        });

        connect(btnSort, &QPushButton::clicked, this, [this]() {
            m_model->sortByTime();
            m_map2d->setRecords(m_model->records());
            m_globe3d->setRecords(m_model->records());
            statusBar()->showMessage("Sorted chronologically by Time (AM/PM) and updated 2D/3D paths.");
        });

        connect(btnGeo, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getSaveFileName(this, "Save GeoJSON", "flight_sorted.geojson", "GeoJSON (*.geojson)");
            if (path.isEmpty()) return;
            if (FlightDataManager::writeGeoJSON(m_model->records(), path)) {
                statusBar()->showMessage("Exported GeoJSON with ISO-8601 timestamps.");
            }
        });

        connect(btnBin, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Output Folder for Binary Files");
            if (dir.isEmpty()) return;
            FlightDataManager::writeBinaryFilesAsync(
                m_model->records(),
                dir + "/course_lt_220.bin",
                dir + "/alt_37900_38000.bin"
            );
            statusBar()->showMessage("Wrote 2 binary files (3000 bytes each) concurrently via std::async.");
        });

        connect(m_map2d, &Map2DWidget::viewCenterChanged, this, [this](double lat, double lon, double zoom) {
            if (m_syncing) return;
            m_syncing = true;
            m_globe3d->setViewCenter(lat, lon, zoom);
            statusBar()->showMessage(QString("Synchronized Center — Lat: %1°, Lon: %2° | Zoom: %3x")
                .arg(lat, 0, 'f', 2).arg(lon, 0, 'f', 2).arg(zoom, 0, 'f', 2));
            m_syncing = false;
        });

        connect(m_globe3d, &Globe3DWidget::viewCenterChanged, this, [this](double lat, double lon, double zoom) {
            if (m_syncing) return;
            m_syncing = true;
            m_map2d->setViewCenter(lat, lon, zoom);
            statusBar()->showMessage(QString("Synchronized Center — Lat: %1°, Lon: %2° | Zoom: %3x")
                .arg(lat, 0, 'f', 2).arg(lon, 0, 'f', 2).arg(zoom, 0, 'f', 2));
            m_syncing = false;
        });
    }

private:
    QTableView* m_table = nullptr;
    FlightTableModel* m_model = nullptr;
    Map2DWidget* m_map2d = nullptr;
    Globe3DWidget* m_globe3d = nullptr;
    bool m_syncing = false;
};