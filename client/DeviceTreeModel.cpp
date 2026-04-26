#include "DeviceTreeModel.h"
#include <QFont>
#include <QBrush>
#include <QApplication>

DeviceTreeModel::DeviceTreeModel(QObject* parent)
    : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({ "Объект / Параметр" });
}

void DeviceTreeModel::refreshStructure() {
    this->removeRows(0, this->rowCount());
    m_deviceItems.clear();

    if (!DatabaseManager::instance().open()) return;

    // 1. Получаем список всех комнат из БД
    QList<RoomInfo> rooms = DatabaseManager::instance().getRooms();

    // Добавляем виртуальную комнату для устройств без назначенной комнаты
    RoomInfo unassigned = { 0, tr("Без комнаты") };
    rooms.append(unassigned);

    for (const auto& room : rooms) {
        // 2. Получаем устройства, принадлежащие конкретной комнате
        QList<DeviceInfo> devices = DatabaseManager::instance().getDevicesByRoom(room.id);

        // Если в комнате нет устройств, можем пропустить её (кроме "Без комнаты", если нужно)
        if (devices.isEmpty() && room.id != 0) continue;

        // Создаем элемент комнаты (верхний уровень)
        QStandardItem* roomItem = new QStandardItem(room.name);
        roomItem->setEditable(false);
        roomItem->setData(room.id, Qt::UserRole);
        roomItem->setBackground(QBrush(QColor(240, 240, 240))); // Визуальное выделение

        for (const auto& device : devices) {
            // Создаем элемент устройства (второй уровень)
            QStandardItem* devNameItem = new QStandardItem(device.name);
            devNameItem->setData(device.id, Qt::UserRole);
            devNameItem->setToolTip(tr("ID: %1").arg(device.uniqueId));


            // 3. Получаем датчики этого устройства
            QList<SensorInfo> sensors = DatabaseManager::instance().getSensorsForDevice(device.id);
            for (const auto& sensor : sensors) {
                // Элемент с названием датчика
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

    // Получаем все устройства из базы
    QList<DeviceInfo> dbDevices = DatabaseManager::instance().getAllDevices();

    for (const auto& device : dbDevices) {
        // Если устройства нет в нашей мапе, значит оно новое
        if (!m_deviceItems.contains(device.id)) {

            // Определяем родителя (комнату)
            QStandardItem* parentRoom = nullptr;

            if (device.roomId > 0 && m_roomItems.contains(device.roomId)) {
                parentRoom = m_roomItems[device.roomId];
            }
            else {
                // Если комнаты нет в мапе или roomId == 0, берем "Без комнаты"
                parentRoom = m_roomItems.value(0);
            }

            // Если даже "Без комнаты" не создана (такое бывает при пустой базе), создаем её
            if (!parentRoom) {
                parentRoom = new QStandardItem(tr("Без комнаты"));
                parentRoom->setData(0, Qt::UserRole);
                this->appendRow(parentRoom);
                m_roomItems.insert(0, parentRoom);
            }

            // Создаем само устройство
            QStandardItem* devItem = new QStandardItem(device.name);
            devItem->setData(device.id, Qt::UserRole);

            // Сразу ставим иконку (по умолчанию Offline)
            devItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogNoButton));

            // Добавляем в дерево и в мапу
            parentRoom->appendRow(devItem);
            m_deviceItems.insert(device.id, devItem);

            // Сразу подгружаем его датчики, чтобы дерево было полным
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