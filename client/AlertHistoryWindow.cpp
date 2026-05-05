#include "AlertHistoryWindow.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include "general/DatabaseManager.h"

AlertHistoryWindow::AlertHistoryWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("История аномалий");
    resize(900, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QTableWidget* table = new QTableWidget(this);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({ "Время", "Устройство", "Датчик", "Значение", "Ед. изм.", "Тип" });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Только чтение

    // Загружаем данные
    auto history = DatabaseManager::instance().getAlertHistory(100);
    table->setRowCount(history.size());

    for (int i = 0; i < history.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(history[i].timestamp));
        table->setItem(i, 1, new QTableWidgetItem(history[i].deviceName));
        table->setItem(i, 2, new QTableWidgetItem(history[i].sensorKey));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(history[i].value, 'f', 2)));
        table->setItem(i, 4, new QTableWidgetItem(history[i].unit));
        table->setItem(i, 5, new QTableWidgetItem(history[i].type));
    }

    layout->addWidget(table);

    QPushButton* btnClose = new QPushButton("Закрыть");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(btnClose);
}