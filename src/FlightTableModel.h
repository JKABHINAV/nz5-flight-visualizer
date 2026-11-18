#pragma once
#include <QAbstractTableModel>
#include <QColor>
#include "FlightData.h"

class FlightTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit FlightTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

    void setRecords(const std::vector<FlightRecord>& recs) {
        beginResetModel();
        m_records = recs;
        endResetModel();
    }

    const std::vector<FlightRecord>& records() const { return m_records; }

    void sortByTime() {
        beginResetModel();
        FlightDataManager::sortByTime(m_records);
        endResetModel();
    }

    int rowCount(const QModelIndex& = QModelIndex()) const override {
        return static_cast<int>(m_records.size());
    }

    int columnCount(const QModelIndex& = QModelIndex()) const override { return 8; }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
        static const QStringList headers = {
            "Time", "Latitude", "Longitude", "Course", "Speed (Kts)", "Speed (Mph)", "Altitude (ft)", "Facility"
        };
        return headers.value(section);
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid()) return {};
        const auto& r = m_records[index.row()];

        if (role == Qt::DisplayRole) {
            switch (index.column()) {
                case 0: return r.timeStr;
                case 1: return QString::number(r.latitude, 'f', 4);
                case 2: return QString::number(r.longitude, 'f', 4);
                case 3: return r.course ? QString::number(*r.course) : "NULL";
                case 4: return r.speedKts ? QString::number(*r.speedKts) : "NULL";
                case 5: return r.speedMph ? QString::number(*r.speedMph) : "NULL";
                case 6: return r.altitudeFeet ? QString::number(*r.altitudeFeet) : "NULL";
                case 7: return r.reportingFacility;
            }
        } else if (role == Qt::ForegroundRole) {
            if ((index.column() == 4 || index.column() == 5) && !r.speedKts) {
                return QColor("#94a3b8"); // Muted gray for NULL cells
            }
            if (index.column() == 0) return QColor("#38bdf8"); // Cyan highlight for Time
        }
        return {};
    }

private:
    std::vector<FlightRecord> m_records;
};