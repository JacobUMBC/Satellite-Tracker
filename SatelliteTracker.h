#pragma once
#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVariantList>
#include <QtQml/qqml.h>

class SatelliteTracker : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double latitude           READ latitude           NOTIFY positionChanged)
    Q_PROPERTY(double longitude          READ longitude          NOTIFY positionChanged)
    Q_PROPERTY(double altitude           READ altitude           NOTIFY positionChanged)
    Q_PROPERTY(double x                  READ x                  NOTIFY positionChanged)
    Q_PROPERTY(double y                  READ y                  NOTIFY positionChanged)
    Q_PROPERTY(double z                  READ z                  NOTIFY positionChanged)
    Q_PROPERTY(double velocity           READ velocity           NOTIFY positionChanged)
    Q_PROPERTY(double earthRotationAngle READ earthRotationAngle NOTIFY timeUpdated)
    Q_PROPERTY(QString statusText        READ statusText         NOTIFY positionChanged)
    Q_PROPERTY(double sunX               READ sunX               NOTIFY timeUpdated)
    Q_PROPERTY(double sunY               READ sunY               NOTIFY timeUpdated)
    Q_PROPERTY(double sunZ               READ sunZ               NOTIFY timeUpdated)
    Q_PROPERTY(QVariantList pastTrack    READ pastTrack          NOTIFY positionChanged)
    Q_PROPERTY(QVariantList futureTrack  READ futureTrack        NOTIFY positionChanged)

public:
    explicit SatelliteTracker(QObject *parent = nullptr);

    double  latitude()           const { return m_latitude; }
    double  longitude()          const { return m_longitude; }
    double  altitude()           const { return m_altitude; }
    double  x()                  const { return m_x; }
    double  y()                  const { return m_y; }
    double  z()                  const { return m_z; }
    double  velocity()           const { return m_velocity; }
    double  earthRotationAngle() const { return m_earthRotationAngle; }
    QString statusText()         const { return m_statusText; }
    double  sunX()               const { return m_sunX; }
    double  sunY()               const { return m_sunY; }
    double  sunZ()               const { return m_sunZ; }
    QVariantList pastTrack()     const { return m_pastTrack; }
    QVariantList futureTrack()   const { return m_futureTrack; }

signals:
    void positionChanged();
    void timeUpdated();

private slots:
    void fetchPosition();
    void onReplyFinished(QNetworkReply *reply);
    void updateEarthRotation();

private:
    void updateCartesian();
    void calculateVelocity(double newLat, double newLon);
    void updateTracks();
    QVariantMap latLonToScene(double lat, double lon, double radius) const;

    QTimer                *m_timer      = nullptr;
    QTimer                *m_clockTimer = nullptr;
    QNetworkAccessManager *m_network    = nullptr;

    double  m_latitude           = 0.0;
    double  m_longitude          = 0.0;
    double  m_altitude           = 408.0;
    double  m_x                  = 0.0;
    double  m_y                  = 0.0;
    double  m_z                  = 0.0;
    double  m_velocity           = 7.66;
    double  m_prevLat            = 0.0;
    double  m_prevLon            = 0.0;
    double  m_earthRotationAngle = 0.0;
    double  m_sunX               = -1.0;
    double  m_sunY               =  0.0;
    double  m_sunZ               =  0.0;
    bool    m_firstUpdate        = true;
    QString m_statusText         = "Connecting...";

    QVariantList m_pastTrack;
    QVariantList m_futureTrack;
    QList<QPair<double,double>> m_positionHistory;

    static constexpr double ORBIT_RADIUS          = 200.0; // ← pushed out further
    static constexpr double EARTH_RADIUS_KM       = 6371.0;
    static constexpr double ISS_ALTITUDE_KM       = 408.0;
    static constexpr int    MAX_PAST_POINTS       = 60;
    static constexpr int    FUTURE_POINTS         = 60;
    static constexpr double ISS_SPEED_DEG_PER_SEC = 0.0659;
};