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

// DatabaseManager.cpp
QList<DeviceInfo> DatabaseManager::getDevicesByRoom(int roomId) {
    QList<DeviceInfo> list;
    QSqlQuery q(m_db);

    if (roomId > 0) {
        q.prepare("SELECT id, name, unique_id FROM devices WHERE room_id = ?");
        q.addBindValue(roomId);
    }
    else {
        // Запрос для нераспределенных устройств
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

// Фрагмент обновления в DatabaseManager.cpp
QList<SensorInfo> DatabaseManager::getSensorsForDevice(int deviceId) {
    QList<SensorInfo> list;
    QSqlQuery q(m_db);

    // Добавляем новые колонки в SELECT
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
            // Заполняем лимиты
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
    Q_UNUSED(templateId); // шаблон пока не используется
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
    // Ищем датчик и сразу берем его текущую единицу измерения
    q.prepare("SELECT id, unit FROM sensors WHERE device_id = ? AND sensor_key = ?");
    q.addBindValue(deviceId);
    q.addBindValue(key);
    q.exec();

    if (q.next()) {
        int sensorId = q.value(0).toInt();
        QString currentUnit = q.value(1).toString();

        // Если устройство прислало нормальную единицу (не "?") 
        // и она отличается от той, что в базе — обновляем базу!
        if (unit != "?" && currentUnit != unit) {
            QSqlQuery updateQ(m_db);
            updateQ.prepare("UPDATE sensors SET unit = ? WHERE id = ?");
            updateQ.addBindValue(unit);
            updateQ.addBindValue(sensorId);
            updateQ.exec();
        }
        return sensorId;
    }

    // Если датчика нет — создаем его с той единицей, что прислало устройство
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

    // 1. Получаем ID устройства и его человеческое имя для отчетов
    int deviceId = getOrCreateDevice(uniqueId);
    QString deviceName = "Неизвестное устройство";

    QSqlQuery qDev(m_db);
    qDev.prepare("SELECT name FROM devices WHERE id = ?");
    qDev.addBindValue(deviceId);
    if (qDev.exec() && qDev.next()) {
        deviceName = qDev.value(0).toString();
    }

    // 2. Обрабатываем каждый датчик в JSON[cite: 1]
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

        // Получаем ID датчика (и обновляем unit, если пришел новый)[cite: 1]
        int sensorId = getOrCreateSensor(deviceId, sensorKey, unit);

        // --- БЛОК АНАЛИТИКИ ---

        // Запрашиваем пороги и последние данные одним запросом
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

            // А) Проверка абсолютных границ
            if (!minLimit.isNull() && newValue < minLimit.toDouble()) {
                saveAlert(sensorId, "MIN_LIMIT", newValue);
                emit anomalyDetected(uniqueId, deviceName, sensorKey, newValue, "MIN_LIMIT");
            }
            if (!maxLimit.isNull() && newValue > maxLimit.toDouble()) {
                saveAlert(sensorId, "MAX_LIMIT", newValue);
                emit anomalyDetected(uniqueId, deviceName, sensorKey, newValue, "MAX_LIMIT");
            }

            // Б) Анализ скорости изменения
            // Формула: $$Rate = \frac{|V_{new} - V_{old}|}{\Delta t}$$
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
        }

        // 3. Сохраняем новое значение в историю[cite: 1]
        QSqlQuery qInsert(m_db);
        qInsert.prepare("INSERT INTO sensor_data (sensor_id, value) VALUES (?, ?)");
        qInsert.addBindValue(sensorId);
        qInsert.addBindValue(newValue);
        qInsert.exec();
    }
}

bool DatabaseManager::isDeviceOnline(int deviceId, int timeoutSeconds) {
    QSqlQuery q(m_db);
    q.prepare("SELECT timestamp FROM sensor_data sd "
        "JOIN sensors s ON sd.sensor_id = s.id "
        "WHERE s.device_id = ? ORDER BY sd.timestamp DESC LIMIT 1");
    q.addBindValue(deviceId);

    if (q.exec() && q.next()) {
        QDateTime lastSeen = q.value(0).toDateTime();
        lastSeen.setTimeZone(QTimeZone::LocalTime); // Учитываем, что храним локально
        return lastSeen.secsTo(QDateTime::currentDateTime()) < timeoutSeconds;
    }
    return false;
}

QList<DeviceInfo> DatabaseManager::getAllDevices() {
    QList<DeviceInfo> list;
    if (!m_db.isOpen()) return list;

    QSqlQuery q("SELECT id, name, room_id, unique_id FROM devices", m_db);

    while (q.next()) {
        DeviceInfo dev;
        dev.id = q.value(0).toInt();
        dev.name = q.value(1).toString();
        dev.roomId = q.value(2).toInt(); // room_id может быть NULL, тогда вернет 0
        dev.uniqueId = q.value(3).toString();
        list.append(dev);
    }
    return list;
}

SensorInfo DatabaseManager::getSensorSettings(int sensorId) {
    SensorInfo info;
    info.id = sensorId;

    QSqlQuery q(m_db);
    q.prepare("SELECT min_limit, max_limit, max_rate, sensor_key, unit FROM sensors WHERE id = ?");
    q.addBindValue(sensorId);

    if (q.exec() && q.next()) {
        // Если в БД NULL, QVariant::toDouble() вернет 0.0, поэтому используем qQNaN() для индикации пустоты
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
    // Обновляем пороги в таблице sensors
    q.prepare("UPDATE sensors SET min_limit = ?, max_limit = ?, max_rate = ? WHERE id = ?");
    q.addBindValue(minLimit);
    q.addBindValue(maxLimit);
    q.addBindValue(maxRate);
    q.addBindValue(sensorId);

    return q.exec();
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
    // Сложный запрос, чтобы собрать имена устройств и единицы измерения датчиков
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
                q.value(5).toDateTime().toString("dd.MM HH:mm:ss")
                });
        }
    }
    return list;
}