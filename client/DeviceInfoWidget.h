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
#include "UptimeTimelineWidget.h"
#include <QMainWindow>
#include <QDockWidget>

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
    QVector<DeviceStateInterval> fetchDeviceUptime(int deviceId, qint64 rangeStart, qint64 rangeEnd);
    void updateStatistics();
    QWidget* createMetricRow(const QString& name, double min, double max, double avg, double stdDev, const QString& unit);

    QDockWidget* m_statsDock;
    QVBoxLayout* m_statsMainLayout;
    QElapsedTimer m_statsUpdateTimer;
    int m_currentIntervalSeconds = 3600;
    QWidget* m_statsContentWidget;
    int m_currentDeviceId = -1;
    QString m_currentDeviceName;
    QLabel* m_infoLabel;
    QTableWidget* m_sensorsTable;
    QVBoxLayout* m_chartsLayout;

    QComboBox* m_intervalCombo;

    QMainWindow* m_dashboard;             // Внутренний контейнер для перемещаемых виджетов
    QDockWidget* m_timelineDock;          // Блок с таймлайном
    QDockWidget* m_tableDock;
    QMap<int, QDockWidget*> m_chartDocks;
    // Теперь храним просто указатель на наш виджет
    QMap<int, SensorChart*> m_sensorCharts;

    UptimeTimelineWidget* m_timelineWidget;
    QByteArray m_dashboardState;
};

#endif