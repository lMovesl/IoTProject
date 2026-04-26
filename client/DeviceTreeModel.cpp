#include "DeviceTreeModel.h"
#include <QFont>
#include <QBrush>

DeviceTreeModel::DeviceTreeModel(QObject* parent)
    : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({ "Объект / Параметр", "Значение" });
}

void DeviceTreeModel::refreshStructure() {
    this->removeRows(0, this->rowCount());
    m_valueItems.clear();

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
            }

            // Добавляем устройство в комнату
            roomItem->appendRow(devNameItem);
        }

        // Добавляем комнату в корень дерева
        this->appendRow(roomItem);
    }
}

void DeviceTreeModel::updateValues() {
    // Итерируемся по всем сохраненным ячейкам значений
    QMapIterator<int, QStandardItem*> i(m_valueItems);
    while (i.hasNext()) {
        i.next();
        int sensorId = i.key();
        QStandardItem* item = i.value();

        // Получаем последнее значение из БД
        QString val = DatabaseManager::instance().getLastSensorValue(sensorId);

        // Находим единицу измерения (хранится в текущем тексте или можно вытянуть из БД)
        // Для простоты предположим, что мы просто обновляем текст
        item->setText(val);
    }
}

void DeviceTreeModel::updateSensorValue(int sensorId, double value,
                                        const QString& unit,
                                        const QDateTime& timestamp)
{
    if (m_valueItems.contains(sensorId)) {
        QStandardItem* item = m_valueItems[sensorId];
        // Форматируем значение с достаточной точностью
        QString txt = QString::number(value, 'g', 6) + unit;
        item->setText(txt);
        // При желании можно сохранить timestamp в данных элемента:
        // item->setData(timestamp, Qt::UserRole+1);
    }
}