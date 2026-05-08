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
    double minLimit;
    double maxLimit;
    double maxRate;
};

struct AlertRecord {
    QString deviceName;
    QString sensorKey;
    QString type;
    double value;
    QString unit;
    QString timestamp;
};

struct MeasurementEntry {
    QString deviceName;
    QString sensorName;
    double value;
    QString unit;
    QDateTime timestamp;
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

    bool saveAlert(int sensorId, const QString& type, double value);
    QList<AlertRecord> getAlertHistory(int limit = 100);
    QString getDeviceNameByMac(const QString& mac);

    SensorInfo getSensorSettings(int sensorId);
    bool updateSensorThresholds(int sensorId, const QVariant& minLimit, const QVariant& maxLimit, const QVariant& maxRate);
    QList<MeasurementEntry> getAllMeasurementsHistory(int limit);
    double predictFutureValue(int sensorId, int futureSeconds = 300, int pointsCount = 10);
private:
    explicit DatabaseManager(QObject* parent = nullptr);
    QSqlDatabase m_db;
signals:
    void sensorThresholdsChanged(int sensorId, double min, double max);
    void anomalyDetected(const QString& uid, const QString& deviceName, const QString& sensorKey, double value, const QString& type);
};

#endif