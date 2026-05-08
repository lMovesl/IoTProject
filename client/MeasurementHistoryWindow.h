#ifndef MEASUREMENTHISTORYWINDOW_H
#define MEASUREMENTHISTORYWINDOW_H

#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>

class MeasurementHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit MeasurementHistoryWindow(QWidget* parent = nullptr);

private:
    QTableWidget* m_table;
    void loadHistory(); // Метод для загрузки всех данных из БД
};

#endif