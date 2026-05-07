#ifndef DEVICEINFOWIDGET_H
#define DEVICEINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>
#include "SensorChart.h" // Наш новый компонент
#include "DatabaseManager.h"

class DeviceInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceInfoWidget(QWidget* parent = nullptr);
    void setDevice(int deviceId, const QString& name);
    void updateData();

public slots:
    void onIntervalChanged(int index);

private:
    void setupUI();
    void createSensorChart(int sensorId, const QString& name, const QString& unit);
    void updateSensorChartData(int sensorId);
    void clearCharts();

    int m_currentDeviceId = -1;
    QString m_currentDeviceName;
    QLabel* m_infoLabel;
    QTableWidget* m_sensorsTable;
    QVBoxLayout* m_chartsLayout;

    QComboBox* m_intervalCombo;
    int m_currentIntervalSeconds = 3600;

    // Теперь храним просто указатель на наш виджет
    QMap<int, SensorChart*> m_sensorCharts;
};

#endif