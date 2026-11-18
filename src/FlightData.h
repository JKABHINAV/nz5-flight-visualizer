#pragma once
#include <QString>
#include <QTime>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtEndian>
#include <optional>
#include <vector>
#include <algorithm>
#include <future>
#include <cmath>
#include <cstring>

struct FlightRecord {
    QString timeStr;
    double latitude = 0.0;
    double longitude = 0.0;
    std::optional<int> course;
    std::optional<int> speedKts;
    std::optional<int> speedMph;
    std::optional<int> altitudeFeet;
    QString reportingFacility;

    QTime parsedTime() const {
        return QTime::fromString(timeStr.trimmed(), "h:mm:ss ap");
    }
};

#pragma pack(push, 1)
struct BinaryFlightRecord {
    uint8_t hour;          // 24-hour format (1 byte)
    uint8_t minute;        // (1 byte)
    uint8_t sec;           // (1 byte)
    int32_t latitude;      // MSB first (4 bytes)
    int32_t longitude;     // MSB first (4 bytes)
    uint16_t speedKts;     // MSB first (2 bytes)
    uint16_t altitudeFeet; // MSB first (2 bytes)
};
#pragma pack(pop)
static_assert(sizeof(BinaryFlightRecord) == 15, "Struct must be exactly 15 bytes!");

class FlightDataManager {
public:
    static std::vector<FlightRecord> loadCSV(const QString& path) {
        std::vector<FlightRecord> records;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return records;

        QTextStream in(&file);
        bool firstLine = true;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            if (firstLine) { firstLine = false; continue; } // Skip header

            QStringList cols = line.split(',');
            if (cols.size() < 8) continue;

            auto parseOptInt = [](const QString& s) -> std::optional<int> {
                bool ok = false;
                int val = s.trimmed().toInt(&ok);
                return ok ? std::optional<int>(val) : std::nullopt;
            };

            FlightRecord r;
            r.timeStr = cols[0].trimmed();
            r.latitude = cols[1].toDouble();
            r.longitude = cols[2].toDouble();
            r.course = parseOptInt(cols[3]);
            r.speedKts = parseOptInt(cols[4]);
            r.speedMph = parseOptInt(cols[5]);
            r.altitudeFeet = parseOptInt(cols[6]);
            r.reportingFacility = cols[7].trimmed();
            records.push_back(r);
        }
        return records;
    }

    static void sortByTime(std::vector<FlightRecord>& records) {
        std::sort(records.begin(), records.end(), [](const FlightRecord& a, const FlightRecord& b) {
            return a.parsedTime() < b.parsedTime();
        });
    }

    static bool writeGeoJSON(const std::vector<FlightRecord>& records, const QString& path) {
        QJsonArray features;
        QJsonArray lineCoords;
        QDate today = QDate::currentDate();

        for (const auto& r : records) {
            QDateTime dt(today, r.parsedTime());
            QJsonObject props;
            props["Time"] = dt.toString(Qt::ISODate);
            if (r.course) props["Course"] = *r.course;
            if (r.speedKts) props["SpeedKts"] = *r.speedKts;
            if (r.speedMph) props["SpeedMph"] = *r.speedMph;
            if (r.altitudeFeet) props["AltitudeFeet"] = *r.altitudeFeet;
            props["ReportingFacility"] = r.reportingFacility;

            QJsonArray coords{ r.longitude, r.latitude }; // GeoJSON uses [lon, lat]
            lineCoords.append(coords);

            QJsonObject geom{ {"type", "Point"}, {"coordinates", coords} };
            features.append(QJsonObject{ {"type", "Feature"}, {"geometry", geom}, {"properties", props} });
        }

        if (!lineCoords.isEmpty()) {
            QJsonObject routeGeom{ {"type", "LineString"}, {"coordinates", lineCoords} };
            features.append(QJsonObject{
                {"type", "Feature"},
                {"geometry", routeGeom},
                {"properties", QJsonObject{{"name", "Flight Trajectory"}}}
            });
        }

        QJsonObject root{ {"type", "FeatureCollection"}, {"features", features} };
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly)) return false;
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        return true;
    }

    static BinaryFlightRecord toBigEndianStruct(const FlightRecord& r) {
        BinaryFlightRecord b{};
        QTime t = r.parsedTime();
        b.hour = static_cast<uint8_t>(t.hour());
        b.minute = static_cast<uint8_t>(t.minute());
        b.sec = static_cast<uint8_t>(t.second());

        constexpr double SCALE = 8.381903e-8;
        int32_t latScaled = static_cast<int32_t>(std::llround(r.latitude / SCALE));
        int32_t lonScaled = static_cast<int32_t>(std::llround(r.longitude / SCALE));

        b.latitude = qToBigEndian(latScaled);
        b.longitude = qToBigEndian(lonScaled);
        b.speedKts = qToBigEndian(static_cast<uint16_t>(r.speedKts.value_or(0)));
        b.altitudeFeet = qToBigEndian(static_cast<uint16_t>(r.altitudeFeet.value_or(0)));
        return b;
    }

    static void writeBinaryFilesAsync(std::vector<FlightRecord> records,
                                      const QString& file1Path,
                                      const QString& file2Path) {
        // Task 4: Use std::async for sort and concurrent file writes (exact 3000 bytes each)
        auto sortFuture = std::async(std::launch::async, [&records]() {
            sortByTime(records);
        });
        sortFuture.get();

        // Take first 200 rows
        size_t limit = std::min<size_t>(200, records.size());
        std::vector<FlightRecord> first200(records.begin(), records.begin() + limit);

        auto writeFiltered = [](const std::vector<FlightRecord>& subset,
                                const QString& path,
                                auto predicate) -> bool {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return false;

            int count = 0;
            for (const auto& r : subset) {
                if (count >= 200) break;
                if (predicate(r)) {
                    BinaryFlightRecord bin = toBigEndianStruct(r);
                    file.write(reinterpret_cast<const char*>(&bin), sizeof(BinaryFlightRecord));
                    count++;
                }
            }
            // Pad remaining bytes with zeros so file is strictly 200 * 15 = 3000 bytes
            BinaryFlightRecord empty{};
            while (count < 200) {
                file.write(reinterpret_cast<const char*>(&empty), sizeof(BinaryFlightRecord));
                count++;
            }
            return true;
        };

        auto f1 = std::async(std::launch::async, writeFiltered, first200, file1Path,
            [](const FlightRecord& r) { return r.course.has_value() && *r.course < 220; });

        auto f2 = std::async(std::launch::async, writeFiltered, first200, file2Path,
            [](const FlightRecord& r) {
                return r.altitudeFeet.has_value() && *r.altitudeFeet > 37900 && *r.altitudeFeet <= 38000;
            });

        f1.get();
        f2.get();
    }
};