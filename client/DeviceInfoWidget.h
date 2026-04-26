#ifndef DEVICEINFOWIDGET_H
#define DEVICEINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>

// Библиотека Qt Charts
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#include "DatabaseManager.h"

// Вспомогательная структура для управления графиком конкретного датчика
struct SensorChartBundle {
    QChartView* view;
    QLineSeries* series;
    QDateTimeAxis* axisX;
    QValueAxis* axisY;
};

class DeviceInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit DeviceInfoWidget(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает текущее устройство для отображения.
     * Вызывается при клике в дереве.
     */
    void setDevice(int deviceId, const QString& name);

    /**
     * @brief Обновляет данные: таблицу (значения + ед. изм.), расчет среднего за час
     * и точки на графиках. Вызывается по таймеру из MainWindow.
     */
    void updateData();

private:
    void setupUI();

    // Создает новый виджет графика, если его еще нет для этого датчика
    void createSensorChart(int sensorId, const QString& name, const QString& unit);
    void updateSensorChartData(int sensorId);
    // Очищает все графики при смене устройства
    void clearCharts();

    int m_currentDeviceId = -1;
    QString m_currentDeviceName;

    // Элементы интерфейса
    QLabel* m_infoLabel;
    QTableWidget* m_sensorsTable;
    QVBoxLayout* m_chartsLayout; // Слой внутри ScrollArea

    // Хранилище графиков: ключ — sensor_id
    QMap<int, SensorChartBundle*> m_sensorCharts;
};

#endif // DEVICEINFOWIDGET_H