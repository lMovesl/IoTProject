#include "DeviceTreeModel.h"
#include <QFont>
#include <QBrush>
#include <QApplication>

DeviceTreeModel::DeviceTreeModel(QObject* parent)
    : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({ "Объект / Параметр" });
}

Qt::DropActions DeviceTreeModel::supportedDragActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

void DeviceTreeModel::refreshStructure() {
    this->removeRows(0, this->rowCount());
    m_deviceItems.clear();

    if (!DatabaseManager::instance().open()) return;

    QList<RoomInfo> rooms = DatabaseManager::instance().getRooms();

    RoomInfo unassigned = { 0, tr("Без комнаты") };
    rooms.append(unassigned);

    for (const auto& room : rooms) {
        QList<DeviceInfo> devices = DatabaseManager::instance().getDevicesByRoom(room.id);

        if (devices.isEmpty() && room.id != 0) continue;

        QStandardItem* roomItem = new QStandardItem(room.name);
        roomItem->setEditable(false);
        roomItem->setData(room.id, Qt::UserRole);
        roomItem->setBackground(QBrush(QColor(240, 240, 240))); 

        for (const auto& device : devices) {
            QStandardItem* devNameItem = new QStandardItem(device.name);
            devNameItem->setData(device.id, Qt::UserRole);
            devNameItem->setToolTip(tr("ID: %1").arg(device.uniqueId));
            devNameItem->setFlags(devNameItem->flags() | Qt::ItemIsDragEnabled);
            QList<SensorInfo> sensors = DatabaseManager::instance().getSensorsForDevice(device.id);
            for (const auto& sensor : sensors) {
                QStandardItem* sensorNameItem = new QStandardItem(sensor.key);
                sensorNameItem->setData(device.id, Qt::UserRole);
                sensorNameItem->setData(sensor.id, Qt::UserRole + 1);

                devNameItem->appendRow(sensorNameItem);
                m_sensorItems.insert(sensor.id, sensorNameItem);
            }
            roomItem->appendRow(devNameItem);
            m_deviceItems.insert(device.id, devNameItem);
        }
        this->appendRow(roomItem);
        m_roomItems.insert(room.id, roomItem);
    }

    updateDeviceStatuses();
}

void DeviceTreeModel::updateDeviceStatuses() {
    QMap<int, QStandardItem*>::iterator it;
    for (it = m_deviceItems.begin(); it != m_deviceItems.end(); ++it) {
        int deviceId = it.key();
        QStandardItem* devItem = it.value();

        if (!devItem) continue;

        bool isOnline = DatabaseManager::instance().isDeviceOnline(deviceId);

        QIcon statusIcon = isOnline ?
            QApplication::style()->standardIcon(QStyle::SP_DialogYesButton) :
            QApplication::style()->standardIcon(QStyle::SP_DialogNoButton);

        devItem->setIcon(statusIcon);
    }
}


void DeviceTreeModel::syncDevicesFromDb() {
    if (!DatabaseManager::instance().open()) return;

    QList<DeviceInfo> dbDevices = DatabaseManager::instance().getAllDevices();

    for (const auto& device : dbDevices) {
        if (!m_deviceItems.contains(device.id)) {

            QStandardItem* parentRoom = nullptr;

            if (device.roomId > 0 && m_roomItems.contains(device.roomId)) {
                parentRoom = m_roomItems[device.roomId];
            }
            else {
                parentRoom = m_roomItems.value(0);
            }

            if (!parentRoom) {
                parentRoom = new QStandardItem(tr("Без комнаты"));
                parentRoom->setData(0, Qt::UserRole);
                this->appendRow(parentRoom);
                m_roomItems.insert(0, parentRoom);
            }

            QStandardItem* devItem = new QStandardItem(device.name);
            devItem->setData(device.id, Qt::UserRole);

            devItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogNoButton));

            parentRoom->appendRow(devItem);
            m_deviceItems.insert(device.id, devItem);

            loadSensorsForNewDevice(devItem, device.id);
        }
    }
}

void DeviceTreeModel::loadSensorsForNewDevice(QStandardItem* devItem, int deviceId) {
    QList<SensorInfo> sensors = DatabaseManager::instance().getSensorsForDevice(deviceId);
    for (const auto& sensor : sensors) {
        if (!m_sensorItems.contains(sensor.id)) {
            QStandardItem* sensorItem = new QStandardItem(sensor.key);
            sensorItem->setData(deviceId, Qt::UserRole);
            sensorItem->setData(sensor.id, Qt::UserRole + 1);

            devItem->appendRow(sensorItem);
            m_sensorItems.insert(sensor.id, sensorItem);
        }
    }
}