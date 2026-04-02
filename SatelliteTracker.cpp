#include "SatelliteTracker.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QtMath>
#include <QDateTime>
#include <QTimeZone>
#include <cmath>

SatelliteTracker::SatelliteTracker(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_clockTimer(new QTimer(this))
    , m_network(new QNetworkAccessManager(this))
{
    connect(m_timer, &QTimer::timeout, this, &SatelliteTracker::fetchPosition);
    connect(m_network, &QNetworkAccessManager::finished,
            this, &SatelliteTracker::onReplyFinished);
    connect(m_clockTimer, &QTimer::timeout, this, &SatelliteTracker::updateEarthRotation);

    m_timer->start(5000);
    m_clockTimer->start(1000);

    fetchPosition();
    updateEarthRotation();
}

void SatelliteTracker::fetchPosition()
{
    QNetworkRequest request(QUrl("http://api.open-notify.org/iss-now.json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "SatelliteTracker/1.0");
    m_network->get(request);
}

void SatelliteTracker::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_statusText = "Network error: " + reply->errorString();
        emit positionChanged();
        return;
    }

    const QByteArray    data = reply->readAll();
    const QJsonDocument doc  = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        m_statusText = "Invalid response";
        emit positionChanged();
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonObject pos  = root["iss_position"].toObject();

    double newLat = pos["latitude"].toString().toDouble();
    double newLon = pos["longitude"].toString().toDouble();

    if (!m_firstUpdate)
        calculateVelocity(newLat, newLon);

    m_latitude    = newLat;
    m_longitude   = newLon;
    m_altitude    = ISS_ALTITUDE_KM;
    m_firstUpdate = false;

    m_positionHistory.append({newLat, newLon});
    if (m_positionHistory.size() > MAX_PAST_POINTS)
        m_positionHistory.removeFirst();

    updateCartesian();
    updateTracks();

    m_statusText = QString("Lat: %1°  Lon: %2°  |  %3 km/s")
                       .arg(m_latitude,  0, 'f', 2)
                       .arg(m_longitude, 0, 'f', 2)
                       .arg(m_velocity,  0, 'f', 2);

    emit positionChanged();
}

QVariantMap SatelliteTracker::latLonToScene(double lat, double lon, double radius) const
{
    const double latRad = qDegreesToRadians(lat);
    const double lonRad = qDegreesToRadians(lon);
    QVariantMap point;
    point["x"] =  radius * qCos(latRad) * qCos(lonRad);
    point["y"] =  radius * qSin(latRad);
    point["z"] = -radius * qCos(latRad) * qSin(lonRad);
    return point;
}

void SatelliteTracker::updateTracks()
{
    // --- Past track at orbit height ---
    m_pastTrack.clear();
    for (int i = 0; i < m_positionHistory.size(); ++i) {
        auto [lat, lon] = m_positionHistory[i];
        QVariantMap pt = latLonToScene(lat, lon, ORBIT_RADIUS);
        pt["opacity"] = 0.1 + 0.9 * (double(i) / double(m_positionHistory.size()));
        m_pastTrack.append(pt);
    }

    // --- Future track at orbit height ---
    m_futureTrack.clear();

    double headingRad = 0.0;
    if (m_positionHistory.size() >= 2) {
        auto [lat1, lon1] = m_positionHistory[m_positionHistory.size() - 2];
        auto [lat2, lon2] = m_positionHistory[m_positionHistory.size() - 1];
        headingRad = qAtan2(
            qDegreesToRadians(lon2 - lon1),
            qDegreesToRadians(lat2 - lat1)
            );
    }

    double predLat = m_latitude;
    double predLon = m_longitude;
    const double stepDeg = ISS_SPEED_DEG_PER_SEC * 5.0;

    for (int i = 0; i < FUTURE_POINTS; ++i) {
        predLat += stepDeg * qCos(headingRad);
        predLon += stepDeg * qSin(headingRad);

        if (predLon >  180.0) predLon -= 360.0;
        if (predLon < -180.0) predLon += 360.0;
        predLat = qBound(-85.0, predLat, 85.0);

        QVariantMap pt = latLonToScene(predLat, predLon, ORBIT_RADIUS);
        pt["opacity"] = 0.7 - 0.6 * (double(i) / double(FUTURE_POINTS));
        m_futureTrack.append(pt);
    }
}

void SatelliteTracker::calculateVelocity(double newLat, double newLon)
{
    const double lat1 = qDegreesToRadians(m_latitude);
    const double lat2 = qDegreesToRadians(newLat);
    const double dLat = qDegreesToRadians(newLat - m_latitude);
    const double dLon = qDegreesToRadians(newLon - m_longitude);

    const double a = qSin(dLat/2) * qSin(dLat/2) +
                     qCos(lat1) * qCos(lat2) *
                         qSin(dLon/2) * qSin(dLon/2);
    const double c = 2 * qAtan2(qSqrt(a), qSqrt(1-a));
    const double distanceKm = (EARTH_RADIUS_KM + ISS_ALTITUDE_KM) * c;

    m_velocity = distanceKm / 5.0;
}

void SatelliteTracker::updateEarthRotation()
{
    const QDateTime utc = QDateTime::currentDateTimeUtc();

    const int secondsInDay = utc.time().hour()   * 3600
                             + utc.time().minute() * 60
                             + utc.time().second();
    m_earthRotationAngle = (secondsInDay / 86400.0) * 360.0;

    const QDateTime j2000 = QDateTime(QDate(2000, 1, 1), QTime(12, 0, 0), QTimeZone::utc());
    const double daysSinceJ2000 = j2000.msecsTo(utc) / 86400000.0;

    const double meanLon  = fmod(280.460 + 0.9856474 * daysSinceJ2000, 360.0);
    const double meanAnom = qDegreesToRadians(fmod(357.528 + 0.9856003 * daysSinceJ2000, 360.0));
    const double eclipticLon = qDegreesToRadians(meanLon + 1.915 * qSin(meanAnom)
                                                 + 0.020 * qSin(2.0 * meanAnom));
    const double obliquity = qDegreesToRadians(23.44);

    m_sunX = qCos(eclipticLon);
    m_sunY = qSin(eclipticLon) * qCos(obliquity);
    m_sunZ = qSin(eclipticLon) * qSin(obliquity);

    emit timeUpdated();
}

void SatelliteTracker::updateCartesian()
{
    const double latRad = qDegreesToRadians(m_latitude);
    const double lonRad = qDegreesToRadians(m_longitude);
    const double r      = ORBIT_RADIUS;

    m_x =  r * qCos(latRad) * qCos(lonRad);
    m_y =  r * qSin(latRad);
    m_z = -r * qCos(latRad) * qSin(lonRad);
}