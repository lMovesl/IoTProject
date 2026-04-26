#include "DeviceInfoWidget.h"

DeviceInfoWidget::DeviceInfoWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    m_nameLabel = new QLabel("Выберите устройство в дереве", this);
    m_nameLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    m_idLabel = new QLabel(this);

    m_sensorsTable = new QTableWidget(0, 2, this);
    m_sensorsTable->setHorizontalHeaderLabels({ "Параметр", "Значение" });
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(m_nameLabel);
    layout->addWidget(m_idLabel);
    layout->addWidget(m_sensorsTable);
}

void DeviceInfoWidget::setDevice(int deviceId, const QString& deviceName)
{
    m_deviceId = deviceId;
    m_deviceName = deviceName;
}

void DeviceInfoWidget::displayDevice() {
    m_nameLabel->setText(m_deviceName);
    m_idLabel->setText(QString("ID устройства: %1").arg(m_deviceId));

    m_sensorsTable->setRowCount(0);
    auto sensors = DatabaseManager::instance().getSensorsForDevice(m_deviceId);

    for (const auto& s : sensors) {
        int row = m_sensorsTable->rowCount();
        m_sensorsTable->insertRow(row);
        m_sensorsTable->setItem(row, 0, new QTableWidgetItem(s.key));
        m_sensorsTable->setItem(row, 1, new QTableWidgetItem(s.lastValue + " " + s.unit));
    }
}
