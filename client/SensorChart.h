#ifndef SENSORCHART_H
#define SENSORCHART_H

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QDateTime>

class SensorChart : public QChartView {
    Q_OBJECT
public:
    explicit SensorChart(const QString& title, int sensorId = -1, QWidget* parent = nullptr);

    void addPoint(const QDateTime& time, double value);
    void setThresholds(double min, double max);
    void clearData();
    void setXAxisRange(const QDateTime& start, const QDateTime& end);
    void setPoints(const QList<QPointF>& points);
    void updatePrediction(const QDateTime& currentTime, double currentValue,
        int futureSeconds, double predictedValue);
    void clearPrediction();
    void resetZoom();
public slots:
    void onThresholdsUpdated(int sensorId, double min, double max);
    void handlePointHovered(const QPointF& point, bool state);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QChart* m_chart;
    QLineSeries* m_series;
    QLineSeries* m_minLine;
    QLineSeries* m_maxLine;
    QLineSeries* m_predictSeries;
    QDateTimeAxis* m_axisX;
    QValueAxis* m_axisY;
    int m_sensorId;

    bool m_isZoomed = false;

    void updateThresholdPositions();
};

#endif