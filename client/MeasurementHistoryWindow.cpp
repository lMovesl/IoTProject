#include "MeasurementHistoryWindow.h"
#include "DatabaseManager.h"

MeasurementHistoryWindow::MeasurementHistoryWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("История всех измерений");
    resize(700, 500);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({ "Устройство", "Датчик", "Значение", "Время" });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_table);

    loadHistory();
}

void MeasurementHistoryWindow::loadHistory() {
    m_table->setRowCount(0);

    // Здесь вызываем метод БД (нужно убедиться, что он возвращает данные)
    // SQL: SELECT d.name, s.name, m.value, m.timestamp FROM measurements m 
    // JOIN sensors s ON m.sensor_id = s.id JOIN devices d ON s.device_id = d.id 
    // ORDER BY m.timestamp DESC LIMIT 100
    auto history = DatabaseManager::instance().getAllMeasurementsHistory(100);

    for (const auto& entry : history) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(entry.deviceName));
        m_table->setItem(row, 1, new QTableWidgetItem(entry.sensorName));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(entry.value) + " " + entry.unit));
        m_table->setItem(row, 3, new QTableWidgetItem(entry.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
    }
}