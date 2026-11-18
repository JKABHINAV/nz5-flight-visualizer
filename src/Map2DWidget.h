#pragma once
#define _USE_MATH_DEFINES
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include "FlightData.h"
#include "MBTilesLoader.h"

class Map2DWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit Map2DWidget(const QString& mbtilesPath, QWidget* parent = nullptr)
        : QOpenGLWidget(parent), m_mbtilesPath(mbtilesPath) {}

    void setRecords(const std::vector<FlightRecord>& recs) { m_records = recs; update(); }

    void setViewCenter(double lat, double lon, double zoom) {
        m_centerLat = std::clamp(lat, -85.0, 85.0);
        m_centerLon = lon;
        m_zoom = std::clamp(zoom, 1.0, 12.0);
        update();
    }

signals:
    void viewCenterChanged(double lat, double lon, double zoom);

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.04f, 0.09f, 0.16f, 1.0f);
        QImage img = MBTilesLoader::loadWorldTexture(m_mbtilesPath, 2);
        if (!img.isNull()) {
            m_texture = new QOpenGLTexture(img.mirrored());
            m_texture->setMinificationFilter(QOpenGLTexture::Linear);
            m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        }
    }

    void resizeGL(int w, int h) override { glViewport(0, 0, w, h); }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double halfW = 180.0 / m_zoom, halfH = 180.0 / m_zoom;
        double cy = latToMercY(m_centerLat);
        glOrtho(m_centerLon - halfW, m_centerLon + halfW,
                cy - halfH, cy + halfH, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        if (m_texture) {
            glEnable(GL_TEXTURE_2D);
            m_texture->bind();
            glColor3f(1.0f, 1.0f, 1.0f);
            for (int offset = -360; offset <= 360; offset += 360) {
                glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex2f(-180 + offset, -180);
                glTexCoord2f(1, 0); glVertex2f( 180 + offset, -180);
                glTexCoord2f(1, 1); glVertex2f( 180 + offset,  180);
                glTexCoord2f(0, 1); glVertex2f(-180 + offset,  180);
                glEnd();
            }
            glDisable(GL_TEXTURE_2D);
        }

        if (m_records.size() < 2) return;

        glLineWidth(2.5f);
        glColor3f(1.0f, 0.86f, 0.0f);
        glBegin(GL_LINE_STRIP);
        for (size_t i = 0; i < m_records.size(); ++i) {
            if (i > 0 && std::abs(m_records[i].longitude - m_records[i - 1].longitude) > 180.0) {
                glEnd();
                glBegin(GL_LINE_STRIP);
            }
            glVertex2d(m_records[i].longitude, latToMercY(m_records[i].latitude));
        }
        glEnd();

        drawGeodesic2D(m_records.front(), m_records.back());
    }

    void mousePressEvent(QMouseEvent* e) override { m_lastPos = e->pos(); }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (e->buttons() & Qt::LeftButton) {
            QPoint delta = e->pos() - m_lastPos;
            m_lastPos = e->pos();
            m_centerLon -= delta.x() * (360.0 / m_zoom) / width();
            m_centerLat += delta.y() * (180.0 / m_zoom) / height();
            m_centerLat = std::clamp(m_centerLat, -85.0, 85.0);
            emit viewCenterChanged(m_centerLat, m_centerLon, m_zoom);
            update();
        }
    }

    void wheelEvent(QWheelEvent* e) override {
        m_zoom *= (e->angleDelta().y() > 0) ? 1.15 : 0.87;
        m_zoom = std::clamp(m_zoom, 1.0, 12.0);
        emit viewCenterChanged(m_centerLat, m_centerLon, m_zoom);
        update();
    }

private:
    static double latToMercY(double latDeg) {
        double phi = std::clamp(latDeg, -85.0511, 85.0511) * M_PI / 180.0;
        return std::log(std::tan(M_PI / 4.0 + phi / 2.0)) * 180.0 / M_PI;
    }

    void drawGeodesic2D(const FlightRecord& a, const FlightRecord& b) {
        auto toRad = [](double deg) { return deg * M_PI / 180.0; };
        auto toDeg = [](double rad) { return rad * 180.0 / M_PI; };
        double lat1 = toRad(a.latitude), lon1 = toRad(a.longitude);
        double lat2 = toRad(b.latitude), lon2 = toRad(b.longitude);
        double x1 = cos(lat1)*cos(lon1), y1 = cos(lat1)*sin(lon1), z1 = sin(lat1);
        double x2 = cos(lat2)*cos(lon2), y2 = cos(lat2)*sin(lon2), z2 = sin(lat2);
        double omega = acos(std::clamp(x1*x2 + y1*y2 + z1*z2, -1.0, 1.0));

        glLineWidth(2.5f);
        glColor3f(1.0f, 0.25f, 0.25f);
        glBegin(GL_LINE_STRIP);
        double prevLon = a.longitude;
        for (int i = 0; i <= 100; ++i) {
            double t = i / 100.0;
            double s1 = sin((1 - t) * omega) / sin(omega);
            double s2 = sin(t * omega) / sin(omega);
            double x = s1*x1 + s2*x2, y = s1*y1 + s2*y2, z = s1*z1 + s2*z2;
            double lat = toDeg(atan2(z, sqrt(x*x + y*y)));
            double lon = toDeg(atan2(y, x));
            if (i > 0 && std::abs(lon - prevLon) > 180.0) {
                glEnd();
                glBegin(GL_LINE_STRIP);
            }
            glVertex2d(lon, latToMercY(lat));
            prevLon = lon;
        }
        glEnd();
    }

    QString m_mbtilesPath;
    QOpenGLTexture* m_texture = nullptr;
    std::vector<FlightRecord> m_records;
    double m_centerLat = 0.0, m_centerLon = -160.0, m_zoom = 1.5;
    QPoint m_lastPos;
};
