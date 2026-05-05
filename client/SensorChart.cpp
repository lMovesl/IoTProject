#include "SensorChart.h"
#include "DatabaseManager.h"

SensorChart::SensorChart(const QString& title, int sensorId, QWidget* parent) : QChartView(parent), m_sensorId(sensorId) {
    m_chart = new QChart();
    m_chart->setTitle(title);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_series = new QLineSeries();
    m_series->setName("Значение");

    m_minLine = new QLineSeries();
    m_minLine->setName("Мин. порог");
    QPen blueDash(Qt::blue);
    blueDash.setStyle(Qt::DashLine);
    m_minLine->setPen(blueDash);

    m_maxLine = new QLineSeries();
    m_maxLine->setName("Макс. порог");
    QPen redDash(Qt::red);
    redDash.setStyle(Qt::DashLine);
    m_maxLine->setPen(redDash);

    m_chart->addSeries(m_series);
    m_chart->addSeries(m_minLine);
    m_chart->addSeries(m_maxLine);

    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat("HH:mm:ss");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY = new QValueAxis();
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_minLine->attachAxis(m_axisX);
    m_minLine->attachAxis(m_axisY);
    m_maxLine->attachAxis(m_axisX);
    m_maxLine->attachAxis(m_axisY);

    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);

    if (m_sensorId != -1) {
        connect(&DatabaseManager::instance(), &DatabaseManager::sensorThresholdsChanged,
            this, &SensorChart::onThresholdsUpdated);
    }
}

void SensorChart::setThresholds(double min, double max) {
    m_minLine->clear();
    m_maxLine->clear();

    if (!std::isnan(min)) m_minLine->append(0, min); // Временные точки, обновятся в update
    if (!std::isnan(max)) m_maxLine->append(0, max);

    updateThresholdPositions();
}

void SensorChart::addPoint(const QDateTime& time, double value) {
    qint64 msecs = time.toMSecsSinceEpoch();
    m_series->append(msecs, value);

    // Логика масштабирования
    if (m_series->count() == 1) {
        m_axisX->setRange(time, time.addSecs(30));
        m_axisY->setRange(value - 10, value + 10);
    }
    else {
        if (time > m_axisX->max()) m_axisX->setMax(time);
        if (value > m_axisY->max()) m_axisY->setMax(value + 2);
        if (value < m_axisY->min()) m_axisY->setMin(value - 2);
    }
    updateThresholdPositions();
}

void SensorChart::updateThresholdPositions() {
    qint64 xMin = m_axisX->min().toMSecsSinceEpoch();
    qint64 xMax = m_axisX->max().toMSecsSinceEpoch();

    if (m_minLine->count() > 0) {
        double val = m_minLine->at(0).y();
        m_minLine->replace({ QPointF(xMin, val), QPointF(xMax, val) });
    }
    if (m_maxLine->count() > 0) {
        double val = m_maxLine->at(0).y();
        m_maxLine->replace({ QPointF(xMin, val), QPointF(xMax, val) });
    }
}

void SensorChart::setXAxisRange(const QDateTime& start, const QDateTime& end) {
    m_axisX->setRange(start, end);
    updateThresholdPositions();
}

void SensorChart::setPoints(const QList<QPointF>& points) {
    m_series->replace(points);

    // Автомасштаб по вертикали (Y)
    if (!points.isEmpty()) {
        double minY = points.first().y();
        double maxY = points.first().y();
        for (const auto& p : points) {
            if (p.y() < minY) minY = p.y();
            if (p.y() > maxY) maxY = p.y();
        }
        double padding = (maxY - minY) * 0.15;
        if (padding == 0) padding = 1.0;
        m_axisY->setRange(minY - padding, maxY + padding);
    }
    updateThresholdPositions();
}

void SensorChart::onThresholdsUpdated(int sensorId, double min, double max) {
    if (sensorId == m_sensorId) {
        setThresholds(min, max);
    }
}