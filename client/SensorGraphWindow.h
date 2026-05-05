#ifndef SENSORGRAPHWINDOW_H
#define SENSORGRAPHWINDOW_H

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QTimer>
#include <QDateTimeEdit>
#include <QPushButton>
#include "DatabaseManager.h"
#include "SensorChart.h"

class SensorGraphWindow : public QDialog {
    Q_OBJECT
public:
    explicit SensorGraphWindow(int sensorId, const QString& sensorName, QWidget* parent = nullptr);
    ~SensorGraphWindow();

public slots:
    void applyInterval();
    void appendPoint(double value, const QDateTime& timestamp);
    void loadData();

private:
    int m_sensorId;
    QTimer* m_timer;
    QDateTime m_lastTimestamp;
    QDateTime m_start;
    QDateTime m_end;
    // UI controls for time interval selection
    QDateTimeEdit* m_startEdit;
    QDateTimeEdit* m_endEdit;
    QPushButton* m_applyBtn;

    SensorChart* m_sensorChart;
};

#endif