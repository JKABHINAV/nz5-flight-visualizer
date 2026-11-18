#pragma once
#define _USE_MATH_DEFINES
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include "FlightData.h"
#include "MBTilesLoader.h"

class Globe3DWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit Globe3DWidget(const QString& mbtilesPath, QWidget* parent = nullptr)
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
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.03f, 0.06f, 0.12f, 1.0f);
        QImage img = MBTilesLoader::loadWorldTexture(m_mbtilesPath, 2);
        if (!img.isNull()) {
            m_texture = new QOpenGLTexture(img);
            m_texture->setMinificationFilter(QOpenGLTexture::Linear);
            m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        }
    }

    void resizeGL(int w, int h) override { glViewport(0, 0, w, h); }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = double(width()) / std::max(1, height());
        double scale = 1.35 / m_zoom;
        glOrtho(-scale * aspect, scale * aspect, -scale, scale, -10.0, 10.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotated(m_centerLat, 1.0, 0.0, 0.0);
        glRotated(-m_centerLon - 90.0, 0.0, 1.0, 0.0);

        drawTexturedSphere();

        if (m_records.size() < 2) return;

        glLineWidth(3.0f);
        glColor3f(1.0f, 0.86f, 0.0f);
        glBegin(GL_LINE_STRIP);
        for (const auto& r : m_records) {
            auto p = latLonToXYZ(r.latitude, r.longitude, 1.012);
            glVertex3d(p[0], p[1], p[2]);
        }
        glEnd();

        drawGeodesic3D(m_records.front(), m_records.back());
    }

    void mousePressEvent(QMouseEvent* e) override { m_lastPos = e->pos(); }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (e->buttons() & Qt::LeftButton) {
            QPoint delta = e->pos() - m_lastPos;
            m_lastPos = e->pos();
            m_centerLon -= delta.x() * 0.4 / m_zoom;
            m_centerLat = std::clamp(m_centerLat + delta.y() * 0.4 / m_zoom, -85.0, 85.0);
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
    std::array<double, 3> latLonToXYZ(double latDeg, double lonDeg, double r = 1.0) {
        double lat = latDeg * M_PI / 180.0;
        double lon = lonDeg * M_PI / 180.0;
        return { r * cos(lat) * cos(lon), r * sin(lat), -r * cos(lat) * sin(lon) };
    }

    static double latToMercV(double latDeg) {
        double phi = std::clamp(latDeg, -85.0511, 85.0511) * M_PI / 180.0;
        return 0.5 - std::log(std::tan(M_PI / 4.0 + phi / 2.0)) / (2.0 * M_PI);
    }

    void drawTexturedSphere() {
        if (m_texture) {
            glEnable(GL_TEXTURE_2D);
            m_texture->bind();
        }
        glColor3f(1.0f, 1.0f, 1.0f);
        const int stacks = 48, slices = 72;
        for (int i = 0; i < stacks; ++i) {
            double lat0 = -85.0511 + 170.1022 * i / stacks;
            double lat1 = -85.0511 + 170.1022 * (i + 1) / stacks;
            double v0 = latToMercV(lat0);
            double v1 = latToMercV(lat1);
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; ++j) {
                double lon = -180.0 + 360.0 * j / slices;
                double u = double(j) / slices;
                auto p0 = latLonToXYZ(lat0, lon, 1.0);
                auto p1 = latLonToXYZ(lat1, lon, 1.0);
                glTexCoord2d(u, v0); glVertex3d(p0[0], p0[1], p0[2]);
                glTexCoord2d(u, v1); glVertex3d(p1[0], p1[1], p1[2]);
            }
            glEnd();
        }
        if (m_texture) glDisable(GL_TEXTURE_2D);
    }

    void drawGeodesic3D(const FlightRecord& a, const FlightRecord& b) {
        auto p1 = latLonToXYZ(a.latitude, a.longitude, 1.015);
        auto p2 = latLonToXYZ(b.latitude, b.longitude, 1.015);
        double dot = std::clamp((p1[0]*p2[0] + p1[1]*p2[1] + p1[2]*p2[2]) / (1.015 * 1.015), -1.0, 1.0);
        double omega = acos(dot);

        glLineWidth(3.0f);
        glColor3f(1.0f, 0.25f, 0.25f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 100; ++i) {
            double t = i / 100.0;
            double s1 = sin((1 - t) * omega) / sin(omega);
            double s2 = sin(t * omega) / sin(omega);
            glVertex3d(s1*p1[0] + s2*p2[0], s1*p1[1] + s2*p2[1], s1*p1[2] + s2*p2[2]);
        }
        glEnd();
    }

    QString m_mbtilesPath;
    QOpenGLTexture* m_texture = nullptr;
    std::vector<FlightRecord> m_records;
    double m_centerLat = 0.0, m_centerLon = -160.0, m_zoom = 1.5;
    QPoint m_lastPos;
};
