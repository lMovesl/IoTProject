#ifndef DEVICETREEMODEL_H
#define DEVICETREEMODEL_H

#include <QStandardItemModel>
#include <QMap>
#include <QDateTime>
#include "DatabaseManager.h"

class DeviceTreeModel : public QStandardItemModel {
    Q_OBJECT

public:
    explicit DeviceTreeModel(QObject* parent = nullptr);

    // Полная перестройка дерева (комнаты и устройства)
    void refreshStructure();

    // Быстрое обновление только значений датчиков (без пересоздания узлов)
    void updateValues();

public slots:
    void updateSensorValue(int sensorId, double value,
                           const QString& unit,
                           const QDateTime& timestamp);

private:

    // Хранилище для быстрого доступа к ячейкам значений:
    // Map <sensor_id, QStandardItem*>
    QMap<int, QStandardItem*> m_valueItems;

    // Вспомогательный метод для создания строки (имя + значение)
    QList<QStandardItem*> createTreeRow(const QString& name, const QString& value,
                                        int id, bool isDevice);
};

#endif // DEVICETREEMODEL_H