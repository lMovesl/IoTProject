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
    Qt::DropActions supportedDragActions() const override;
public slots:
    void refreshStructure();
    void updateDeviceStatuses();
    void syncDevicesFromDb();
private:
    void loadSensorsForNewDevice(QStandardItem* devItem, int deviceId);

    QMap<int, QStandardItem*> m_roomItems;   // <roomId, item>
    QMap<int, QStandardItem*> m_deviceItems; // <deviceId, item>
    QMap<int, QStandardItem*> m_sensorItems; // <sensorId, item>
};

#endif // DEVICETREEMODEL_H