#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {}

bool DatabaseManager::open() {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    }
    else {
        m_db = QSqlDatabase::addDatabase("QMYSQL");
    }

	m_db.setHostName("localhost");
    m_db.setDatabaseName("iot_system");
    m_db.setUserName("root");
    m_db.setPassword("root");

    if (!m_db.open()) {
        qDebug() << "Database Error:" << m_db.lastError().text();
        return false;
    }

    return true;
}

QList<RoomInfo> DatabaseManager::getRooms() {
    QList<RoomInfo> list;
    QSqlQuery q("SELECT id, name FROM rooms", m_db);
    while (q.next()) list.append({ q.value(0).toInt(), q.value(1).toString() });
    return list;
}

bool DatabaseManager::createRoom(const QString& name) {
    QSqlQuery q(m_db);
    q.prepare("INSERT IGNORE INTO rooms (name) VALUES (?)");
    q.addBindValue(name);
    return q.exec();
}

QList<DeviceInfo> DatabaseManager::getDevicesByRoom(int roomId) {
    QList<DeviceInfo> list;
    QSqlQuery q(m_db);

    if (roomId > 0) {
        q.prepare("SELECT id, name, unique_id FROM devices WHERE room_id = ?");
        q.addBindValue(roomId);
    }
    else {
        q.prepare("SELECT id, name, unique_id FROM devices WHERE room_id IS NULL OR room_id = 0");
    }

    if (q.exec()) {
        while (q.next()) {
            list.append({ q.value(0).toInt(), roomId, q.value(1).toString(), q.value(2).toString(), true });
        }
    }
    return list;
}

bool DatabaseManager::updateDeviceConfig(int deviceId, const QString& name, int roomId) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE devices SET name = ?, room_id = ?, is_configured = TRUE WHERE id = ?");
    q.addBindValue(name);
    q.addBindValue(roomId);
    q.addBindValue(deviceId);
    return q.exec();
}

QList<SensorInfo> DatabaseManager::getSensorsForDevice(int deviceId) {
    QList<SensorInfo> list;
    QSqlQuery q(m_db);

    QString query =
        "SELECT s.id, s.sensor_key, s.unit, sd.value, s.min_limit, s.max_limit, s.max_rate "
        "FROM sensors s "
        "LEFT JOIN sensor_data sd ON sd.id = (SELECT id FROM sensor_data WHERE sensor_id = s.id ORDER BY timestamp DESC LIMIT 1) "
        "WHERE s.device_id = ?";

    q.prepare(query);
    q.addBindValue(deviceId);

    if (q.exec()) {
        while (q.next()) {
            SensorInfo info;
            info.id = q.value(0).toInt();
            info.key = q.value(1).toString();
            info.unit = q.value(2).toString();
            info.lastValue = q.value(3).isNull() ? "—" : QString::number(q.value(3).toDouble(), 'f', 2);
            info.minLimit = q.value(4).toDouble();
            info.maxLimit = q.value(5).toDouble();
            info.maxRate = q.value(6).toDouble();
            list.append(info);
        }
    }
    return list;
}

QString DatabaseManager::getLastSensorValue(int sensorId) {
    QSqlQuery q(m_db);
    q.prepare("SELECT value FROM sensor_data WHERE sensor_id = ? ORDER BY id DESC LIMIT 1");
    q.addBindValue(sensorId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return "--";
}

bool DatabaseManager::saveNewDeviceToDb(const QString& name, const QString& uniqueId, int templateId)
{
    Q_UNUSED(templateId);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO devices (name, unique_id, is_configured) VALUES (?, ?, FALSE)");
    q.addBindValue(name);
    q.addBindValue(uniqueId);
    return q.exec();
}

int DatabaseManager::getOrCreateDevice(const QString& uniqueId) {
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM devices WHERE unique_id = ?");
    q.addBindValue(uniqueId);
    q.exec();
    if (q.next()) return q.value(0).toInt();

    q.prepare("INSERT INTO devices (unique_id, name, is_configured) VALUES (?, 'Новое устройство', FALSE)");
    q.addBindValue(uniqueId);
    q.exec();
    return q.lastInsertId().toInt();
}

int DatabaseManager::getOrCreateSensor(int deviceId, const QString& key, const QString& unit) {
    QSqlQuery q(m_db);
    q.prepare("SELECT id, unit FROM sensors WHERE device_id = ? AND sensor_key = ?");
    q.addBindValue(deviceId);
    q.addBindValue(key);
    q.exec();

    if (q.next()) {
        int sensorId = q.value(0).toInt();
        QString currentUnit = q.value(1).toString();

        if (unit != "?" && currentUnit != unit) {
            QSqlQuery updateQ(m_db);
            updateQ.prepare("UPDATE sensors SET unit = ? WHERE id = ?");
            updateQ.addBindValue(unit);
            updateQ.addBindValue(sensorId);
            updateQ.exec();
        }
        return sensorId;
    }

    q.prepare("INSERT INTO sensors (device_id, sensor_key, unit) VALUES (?, ?, ?)");
    q.addBindValue(deviceId);
    q.addBindValue(key);
    q.addBindValue(unit);
    q.exec();

    return q.lastInsertId().toInt();
}

void DatabaseManager::processCombinedJson(const QString& uniqueId, const QByteArray& jsonData) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return; 

    QJsonObject root = doc.object();

    int deviceId = getOrCreateDevice(uniqueId);
    QString deviceName = "Неизвестное устройство";

    QSqlQuery qDev(m_db);
    qDev.prepare("SELECT name FROM devices WHERE id = ?");
    qDev.addBindValue(deviceId);
    if (qDev.exec() && qDev.next()) {
        deviceName = qDev.value(0).toString();
    }

    for (auto it = root.begin(); it != root.end(); ++it) {
        QString sensorKey = it.key();
        double newValue = 0.0;
        QString unit = "?";

        if (it.value().isObject()) {
            QJsonObject obj = it.value().toObject();
            newValue = obj["value"].toDouble();
            if (obj.contains("unit")) {
                unit = obj["unit"].toString();
            }
        }
        else {
            newValue = it.value().toDouble();
        }

        int sensorId = getOrCreateSensor(deviceId, sensorKey, unit);

        QSqlQuery qThreshold(m_db);
        qThreshold.prepare(
            "SELECT s.min_limit, s.max_limit, s.max_rate, "
            "(SELECT value FROM sensor_data WHERE sensor_id = s.id ORDER BY timestamp DESC LIMIT 1), "
            "(SELECT timestamp FROM sensor_data WHERE sensor_id = s.id ORDER BY timestamp DESC LIMIT 1) "
            "FROM sensors s WHERE s.id = ?"
        );
        qThreshold.addBindValue(sensorId);

        if (qThreshold.exec() && qThreshold.next()) {
            QVariant minLimit = qThreshold.value(0);
            QVariant maxLimit = qThreshold.value(1);
            QVariant maxRate = qThreshold.value(2);
            QVariant prevValueVar = qThreshold.value(3);
            QVariant prevTimestampVar = qThreshold.value(4);

            if (!minLimit.isNull() && newValue < minLimit.toDouble()) {
                saveAlert(sensorId, "MIN_LIMIT", newValue);
                emit anomalyDetected(uniqueId, deviceName, sensorKey, newValue, "MIN_LIMIT");
            }
            if (!maxLimit.isNull() && newValue > maxLimit.toDouble()) {
                saveAlert(sensorId, "MAX_LIMIT", newValue);
                emit anomalyDetected(uniqueId, deviceName, sensorKey, newValue, "MAX_LIMIT");
            }

            if (!maxRate.isNull() && !prevValueVar.isNull() && !prevTimestampVar.isNull()) {
                double prevValue = prevValueVar.toDouble();
                QDateTime prevTime = prevTimestampVar.toDateTime();
                qint64 secondsPassed = prevTime.secsTo(QDateTime::currentDateTime());

                if (secondsPassed > 0) {
                    double currentRate = std::abs(newValue - prevValue) / secondsPassed;
                    if (currentRate > maxRate.toDouble()) {
                        saveAlert(sensorId, "RATE_LIMIT", newValue);
                        emit anomalyDetected(uniqueId, deviceName, sensorKey, newValue, "RATE_LIMIT");
                    }
                }
            }

            double predictedValue = predictFutureValue(sensorId, 300, 10);

            if (!std::isnan(predictedValue)) {
                if (!maxLimit.isNull() && newValue <= maxLimit.toDouble() && predictedValue > maxLimit.toDouble()) {
                    saveAlert(sensorId, "PREDICT_MAX", predictedValue);
                    emit anomalyDetected(uniqueId, deviceName, sensorKey, predictedValue, "PREDICT_MAX");
                }
                else if (!minLimit.isNull() && newValue >= minLimit.toDouble() && predictedValue < minLimit.toDouble()) {
                    saveAlert(sensorId, "PREDICT_MIN", predictedValue);
                    emit anomalyDetected(uniqueId, deviceName, sensorKey, predictedValue, "PREDICT_MIN");
                }
            }
        }

        QSqlQuery qInsert(m_db);
        qInsert.prepare("INSERT INTO sensor_data (sensor_id, value) VALUES (?, ?)");
        qInsert.addBindValue(sensorId);
        qInsert.addBindValue(newValue);
        qInsert.exec();
    }
}



int DatabaseManager::getDeviceStatus(int deviceId) {
    QSqlQuery query(m_db);
    // Берем флаг включения и время последней активности
    query.prepare("SELECT is_online, last_seen FROM devices WHERE id = :id");
    query.bindValue(":id", deviceId);

    if (query.exec() && query.next()) {
        bool isEnabled = query.value(0).toBool();
        QDateTime lastSeen = query.value(1).toDateTime();

        // 1. Если устройство выключено пользователем
        if (!isEnabled) {
            return 0; // "Выключено" (Серый/Красный без мигания)
        }

        // 2. Проверяем таймаут (например, 15 секунд)
        if (lastSeen.secsTo(QDateTime::currentDateTime()) < 15) {
            return 1; // "В сети" (Зеленый)
        }
        else {
            return 2; // "Таймаут/Потеряно" (Красный + мигание)
        }
    }
    return -1; // Не найдено
}

QList<DeviceInfo> DatabaseManager::getAllDevices() {
    QList<DeviceInfo> list;
    if (!m_db.isOpen()) return list;

    // Добавляем pos_x и pos_y в запрос
    QSqlQuery q("SELECT id, name, room_id, unique_id, pos_x, pos_y FROM devices", m_db);

    while (q.next()) {
        DeviceInfo dev;
        dev.id = q.value(0).toInt();
        dev.name = q.value(1).toString();
        dev.roomId = q.value(2).toInt();
        dev.uniqueId = q.value(3).toString();
        dev.posX = q.value(4).toDouble(); // Читаем X
        dev.posY = q.value(5).toDouble(); // Читаем Y
        list.append(dev);
    }
    return list;
}

int DatabaseManager::getDeviceIdByMac(const QString& mac) {
    if (!m_db.isOpen()) return -1;

    QSqlQuery query(m_db);
    // Предполагаем, что колонка с уникальным идентификатором называется unique_id
    query.prepare("SELECT id FROM devices WHERE unique_id = :mac");
    query.bindValue(":mac", mac);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    qDebug() << "Устройство с MAC" << mac << "не найдено в базе.";
    return -1;
}

bool DatabaseManager::isDeviceOnline(int deviceId) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    // Выбираем и флаг, и время последнего сообщения
    query.prepare("SELECT is_online, last_seen FROM devices WHERE id = :id");
    query.bindValue(":id", deviceId);

    if (query.exec() && query.next()) {
        bool manualStatus = query.value(0).toBool();
        auto dt = query.value(1).toDateTime();
        dt.setTimeZone(QTimeZone(QTimeZone::LocalTime));
        qint64 lastSeen = dt.toSecsSinceEpoch();
        qint64 now = QDateTime::currentSecsSinceEpoch();

        if (now - lastSeen > 30) {
            return false;
        }

        return manualStatus;
    }
    return false;
}

void DatabaseManager::setDeviceOnline(int deviceId, bool isOnline) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    // Теперь колонка is_online точно существует
    query.prepare("UPDATE devices SET is_online = :status WHERE id = :id");
    query.bindValue(":status", isOnline ? 1 : 0);
    query.bindValue(":id", deviceId);

    if (!query.exec()) {
        qDebug() << "Ошибка при смене статуса устройства:" << query.lastError().text();
    }
}

QString DatabaseManager::getDeviceMac(int deviceId) {
    if (!m_db.isOpen()) return "";

    QSqlQuery query(m_db);
    // В вашей таблице MAC-адрес хранится в unique_id
    query.prepare("SELECT unique_id FROM devices WHERE id = :id");
    query.bindValue(":id", deviceId);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

SensorInfo DatabaseManager::getSensorSettings(int sensorId) {
    SensorInfo info;
    info.id = sensorId;

    QSqlQuery q(m_db);
    q.prepare("SELECT min_limit, max_limit, max_rate, sensor_key, unit FROM sensors WHERE id = ?");
    q.addBindValue(sensorId);

    if (q.exec() && q.next()) {
        info.minLimit = q.value(0).isNull() ? qQNaN() : q.value(0).toDouble();
        info.maxLimit = q.value(1).isNull() ? qQNaN() : q.value(1).toDouble();
        info.maxRate = q.value(2).isNull() ? qQNaN() : q.value(2).toDouble();
        info.key = q.value(3).toString();
        info.unit = q.value(4).toString();
    }
    return info;
}

bool DatabaseManager::updateSensorThresholds(int sensorId, const QVariant& minLimit, const QVariant& maxLimit, const QVariant& maxRate) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE sensors SET min_limit = ?, max_limit = ?, max_rate = ? WHERE id = ?");
    q.addBindValue(minLimit);
    q.addBindValue(maxLimit);
    q.addBindValue(maxRate);
    q.addBindValue(sensorId);

    if (q.exec()) {
        emit sensorThresholdsChanged(sensorId, minLimit.toDouble(), maxLimit.toDouble());
        return true;
    }
    return false;
}

bool DatabaseManager::saveAlert(int sensorId, const QString& type, double value) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO alerts (sensor_id, alert_type, value) VALUES (?, ?, ?)");
    q.addBindValue(sensorId);
    q.addBindValue(type);
    q.addBindValue(value);
    return q.exec();
}

QString DatabaseManager::getDeviceNameByMac(const QString& mac) {
    QSqlQuery q(m_db);
    q.prepare("SELECT name FROM devices WHERE unique_id = ?");
    q.addBindValue(mac);
    if (q.exec() && q.next()) return q.value(0).toString();
    return "Неизвестное устройство";
}

QList<AlertRecord> DatabaseManager::getAlertHistory(int limit) {
    QList<AlertRecord> list;
    QSqlQuery q(m_db);
    QString query = R"(
        SELECT d.name, s.sensor_key, a.alert_type, a.value, s.unit, a.timestamp 
        FROM alerts a
        JOIN sensors s ON a.sensor_id = s.id
        JOIN devices d ON s.device_id = d.id
        ORDER BY a.timestamp DESC 
        LIMIT ?
    )";
    q.prepare(query);
    q.addBindValue(limit);

    if (q.exec()) {
        while (q.next()) {
            list.append({
                q.value(0).toString(),
                q.value(1).toString(),
                q.value(2).toString(),
                q.value(3).toDouble(),
                q.value(4).toString(),
                q.value(5).toDateTime(),
                });
        }
    }
    return list;
}

void DatabaseManager::updateLastSeen(int deviceId) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    qint64 now = QDateTime::currentSecsSinceEpoch();

    // Используем FROM_UNIXTIME для конвертации числа в дату
    query.prepare("UPDATE devices SET last_seen = FROM_UNIXTIME(:ts) WHERE id = :id");
    query.bindValue(":ts", now);
    query.bindValue(":id", deviceId);

    if (!query.exec()) {
        qWarning() << "Ошибка обновления last_seen:" << query.lastError().text();
    }
}

double DatabaseManager::predictFutureValue(int sensorId, int futureSeconds, int pointsCount) {
    QSqlQuery q(m_db);
    q.prepare("SELECT value, UNIX_TIMESTAMP(timestamp) FROM sensor_data "
        "WHERE sensor_id = ? ORDER BY timestamp DESC LIMIT ?");
    q.addBindValue(sensorId);
    q.addBindValue(pointsCount);

    if (!q.exec()) return qQNaN();

    QList<QPointF> data;
    while (q.next()) {
        data.prepend(QPointF(q.value(1).toDouble(), q.value(0).toDouble()));
    }

    if (data.size() < 3) return qQNaN();

    int n = data.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

    double startX = data.first().x();

    for (int i = 0; i < n; ++i) {
        double x = data[i].x() - startX;
        double y = data[i].y();
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    double denominator = (n * sumX2 - sumX * sumX);
    if (denominator == 0) return data.last().y();

    double m = (n * sumXY - sumX * sumY) / denominator;
    double b = (sumY - m * sumX) / n;

    double futureX = (data.last().x() - startX) + futureSeconds;
    return m * futureX + b;  //kx + b
}

QList<MeasurementEntry> DatabaseManager::getAllMeasurementsHistory(int limit) {
    QList<MeasurementEntry> list;
    QSqlQuery query;
    query.prepare("SELECT d.name, s.sensor_key, sd.value, s.unit, sd.timestamp "
        "FROM sensor_data sd "
        "JOIN sensors s ON sd.sensor_id = s.id "
        "JOIN devices d ON s.device_id = d.id "
        "ORDER BY sd.timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            list.append({
                query.value(0).toString(),
                query.value(1).toString(),
                query.value(2).toDouble(),
                query.value(3).toString(),
                query.value(4).toDateTime()
                });
        }
    }
    return list;
}

void DatabaseManager::updateDevicePosition(int id, double x, double y) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE devices SET pos_x = :x, pos_y = :y WHERE id = :id");
    query.bindValue(":x", x);
    query.bindValue(":y", y);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Ошибка сохранения координат:" << query.lastError().text();
    }
}