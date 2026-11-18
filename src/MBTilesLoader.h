#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QImage>
#include <QPainter>
#include <QByteArray>
#include <QMessageBox>

class MBTilesLoader {
public:
    static QImage loadWorldTexture(const QString& mbtilesPath, int zoom = 2) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "mbtiles_conn");
        db.setDatabaseName(mbtilesPath);
        if (!db.open()) {
            QMessageBox::warning(nullptr, "MBTiles Error", "Cannot open DB: " + db.lastError().text());
            return QImage();
        }

        int tilesDrawn = 0;
        QImage worldImg;
        {
            QSqlQuery zCheck(db);
            zCheck.prepare("SELECT COUNT(*) FROM tiles WHERE zoom_level = :z");
            zCheck.bindValue(":z", zoom);
            if (zCheck.exec() && zCheck.next() && zCheck.value(0).toInt() == 0) {
                QSqlQuery minZ("SELECT MIN(zoom_level) FROM tiles", db);
                if (minZ.exec() && minZ.next()) zoom = minZ.value(0).toInt();
            }

            int grid = 1 << zoom;
            int tileSize = 256;
            worldImg = QImage(grid * tileSize, grid * tileSize, QImage::Format_RGBA8888);
            worldImg.fill(QColor("#0b192c"));

            QPainter painter(&worldImg);
            QSqlQuery query(db);
            query.prepare("SELECT tile_column, tile_row, tile_data FROM tiles WHERE zoom_level = :z");
            query.bindValue(":z", zoom);

            if (query.exec()) {
                while (query.next()) {
                    int x = query.value(0).toInt();
                    int tmsY = query.value(1).toInt();
                    int xyzY = (grid - 1) - tmsY;
                    QByteArray bytes = query.value(2).toByteArray();

                    QImage tile = QImage::fromData(bytes);
                    if (!tile.isNull()) {
                        painter.drawImage(x * tileSize, xyzY * tileSize, tile.scaled(tileSize, tileSize));
                        tilesDrawn++;
                    }
                }
            }
            painter.end();
        }
        db.close();
        QSqlDatabase::removeDatabase("mbtiles_conn");

        if (tilesDrawn == 0) {
            QMessageBox::warning(nullptr, "MBTiles Error",
                QString("0 tiles decoded at zoom %1 from:\n%2").arg(zoom).arg(mbtilesPath));
        }
        return worldImg;
    }
};
