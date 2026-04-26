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

QList<SensorInfo> DatabaseManager::getSensorsForDevice(int deviceId) {
    QList<SensorInfo> list;
    QSqlQuery q(m_db);

    // Используем подзапрос для поиска последнего значения по времени (timestamp)
    QString query =
        "SELECT s.id, s.sensor_key, s.unit, sd.value "
        "FROM sensors s "
        "LEFT JOIN sensor_data sd ON sd.id = ("
        "    SELECT id FROM sensor_data "
        "    WHERE sensor_id = s.id "
        "    ORDER BY timestamp DESC, id DESC " // Сначала по времени, потом по ID для точности
        "    LIMIT 1"
        ") "
        "WHERE s.device_id = ?";

    q.prepare(query);
    q.addBindValue(deviceId);

    if (!q.exec()) {
        qDebug() << "SQL Error (getSensors):" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        SensorInfo info;
        info.id = q.value(0).toInt();
        info.key = q.value(1).toString();
        info.unit = q.value(2).toString();

        // Обработка случая, когда данных еще нет (NULL)
        if (q.value(3).isNull()) {
            info.lastValue = "—";
        }
        else {
            info.lastValue = QString::number(q.value(3).toDouble(), 'f', 2); // 2 знака после запятой
        }

        list.append(info);
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
    int deviceId = getOrCreateDevice(uniqueId);

    for (auto it = root.begin(); it != root.end(); ++it) {
        QString sensorKey = it.key();
        double value = 0.0;
        QString unit = "?"; // По умолчанию неизвестно

        // Проверяем: прислало ли устройство объект (с value и unit)
        if (it.value().isObject()) {
            QJsonObject obj = it.value().toObject();
            value = obj["value"].toDouble();
            if (obj.contains("unit")) {
                unit = obj["unit"].toString();
            }
        }
        else {
            // Резервный вариант: устройство прислало просто число {"temp": 25}
            value = it.value().toDouble();
        }

        // Передаем найденную единицу измерения в метод создания/поиска
        int sensorId = getOrCreateSensor(deviceId, sensorKey, unit);

        // Записываем само значение
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO sensor_data (sensor_id, value) VALUES (?, ?)");
        q.addBindValue(sensorId);
        q.addBindValue(value);
        q.exec();
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