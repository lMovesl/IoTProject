#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QtSql>
#include <QObject>
#include <QDebug>

// Основные структуры данных для обмена между классами
struct RoomInfo {
    int id;
    QString name;
};

struct DeviceInfo {
    int id;
    int roomId;
    QString name;
    QString uniqueId;
    bool isConfigured;
};

struct SensorInfo {
    int id;
    QString key;
    QString unit;
    QString lastValue;
};

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance() {
        static DatabaseManager owner; // Создастся один раз при первом вызове
        return owner;
    }
    bool open();
    QSqlDatabase database() { return m_db; }

    // Работа с комнатами
    QList<RoomInfo> getRooms();
    bool createRoom(const QString& name);

    // Работа с устройствами
    QList<DeviceInfo> getDevicesByRoom(int roomId);
    bool updateDeviceConfig(int deviceId, const QString& name, int roomId);
    bool saveNewDeviceToDb(const QString& name, const QString& uniqueId, int templateId);

    // Работа с датчиками
    QList<SensorInfo> getSensorsForDevice(int deviceId);
    QString getLastSensorValue(int sensorId);

    // Серверная часть (Discovery)
    int getOrCreateDevice(const QString& uniqueId);
    int getOrCreateSensor(int deviceId, const QString& key, const QString& unit = "?");
    void processCombinedJson(const QString& uniqueId, const QByteArray& jsonData);
    bool isDeviceOnline(int deviceId, int timeoutSeconds = 300); // По умолчанию 5 минут
    QList<DeviceInfo> getAllDevices();
private:
    explicit DatabaseManager(QObject* parent = nullptr);
    QSqlDatabase m_db;
};

#endif