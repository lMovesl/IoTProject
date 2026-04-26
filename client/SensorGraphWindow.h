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

class SensorGraphWindow : public QDialog {
    Q_OBJECT
public:
    explicit SensorGraphWindow(int sensorId, const QString& sensorName, QWidget* parent = nullptr);
    ~SensorGraphWindow();
private slots:
    void applyInterval();
public slots:
    void appendPoint(double value, const QDateTime& timestamp);

private:
    void loadData();
    void refreshData();
    int m_sensorId;
    QLineSeries* m_series;
    QChart* m_chart;
    QTimer* m_timer;
    QDateTime m_lastTimestamp;
    // UI controls for time interval selection
    QDateTimeEdit* m_startEdit;
    QDateTimeEdit* m_endEdit;
    QPushButton* m_applyBtn;
};

#endif