#include "SensorChart.h"
#include "DatabaseManager.h"

#include <QToolTip>

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

    m_predictSeries = new QLineSeries();
    m_predictSeries->setName("Прогноз (тренд)");
    QPen predictPen(Qt::magenta); // Цвет прогноза
    predictPen.setWidth(3);
    predictPen.setStyle(Qt::DashLine);
    m_predictSeries->setPen(predictPen);

    m_chart->addSeries(m_series);
    m_chart->addSeries(m_minLine);
    m_chart->addSeries(m_maxLine);
    m_chart->addSeries(m_predictSeries);

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
    m_predictSeries->attachAxis(m_axisX);
    m_predictSeries->attachAxis(m_axisY);

    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
    m_series->setPointsVisible(true);
    setMouseTracking(true);

    connect(m_series, &QLineSeries::hovered, this, &SensorChart::handlePointHovered);
    if (m_sensorId != -1) {
        connect(&DatabaseManager::instance(), &DatabaseManager::sensorThresholdsChanged,
            this, &SensorChart::onThresholdsUpdated);
    }

    setRubberBand(QChartView::RectangleRubberBand);
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

    if (!m_isZoomed) {
        m_axisX->setRange(time.addSecs(-600), time.addSecs(180));

        if (value > m_axisY->max() || value < m_axisY->min()) {
            m_axisY->setRange(value - 10, value + 10);
        }
        updateThresholdPositions();
    }
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
    if (m_isZoomed) return;
    
    m_axisX->setRange(start, end);
    updateThresholdPositions();
}

void SensorChart::setPoints(const QList<QPointF>& points) {
    m_series->replace(points);

    // Автомасштаб по вертикали (Y)
    if (!m_isZoomed && !points.isEmpty()) {
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

void SensorChart::updatePrediction(const QDateTime& currentTime, double currentValue,
    int futureSeconds, double predictedValue) {
    if (!m_axisX) return;

    m_predictSeries->clear();
    QDateTime futureTime = currentTime.addSecs(futureSeconds);

    m_predictSeries->append(currentTime.toMSecsSinceEpoch(), currentValue);
    m_predictSeries->append(futureTime.toMSecsSinceEpoch(), predictedValue);

    if (!m_isZoomed) {
        // Если прогноз выходит за текущий видимый диапазон, расширяем его еще чуть дальше
        if (m_axisX->max() < futureTime) {
            m_axisX->setMax(futureTime.addSecs(60)); // Запас в 1 минуту после конца прогноза
        }

        // Корректируем ось Y, если прогноз очень высокий или низкий
        if (predictedValue > m_axisY->max()) m_axisY->setMax(predictedValue + 5);
        if (predictedValue < m_axisY->min()) m_axisY->setMin(predictedValue - 5);
    }
}
void SensorChart::clearPrediction() {
    if (m_predictSeries) {
        m_predictSeries->clear();
    }
}

void SensorChart::handlePointHovered(const QPointF& point, bool state) {
    if (state) {
        // Конвертируем X (ms) в QDateTime
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(point.x());
        QString timeStr = dt.toString("HH:mm:ss");

        // Формируем текст подсказки
        QString tooltipText = QString("Время: %1\nЗначение: %2")
            .arg(timeStr)
            .arg(QString::number(point.y(), 'f', 2));

        // Показываем стандартный QToolTip в позиции курсора
        QToolTip::showText(QCursor::pos(), tooltipText, this);
    }
    else {
        // Прячем подсказку, когда уводим мышь
        QToolTip::hideText();
    }
}

void SensorChart::wheelEvent(QWheelEvent* event) {
    m_isZoomed = true; // Как только пользователь тронул масштаб, автосдвиг отключается

    if (event->angleDelta().y() > 0) {
        chart()->zoomIn();
    }
    else {
        chart()->zoomOut();
    }

    // Если мы вышли на исходный масштаб (zoomOut до упора), можно вернуть m_isZoomed = false,
    // но обычно проще оставить сброс на правую кнопку.
    QChartView::wheelEvent(event);
}

void SensorChart::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        chart()->zoomReset(); // Сброс масштаба
        m_isZoomed = false;   // Возвращаем автосдвиг за новыми данными
    }

    // Если была нажата левая кнопка (начало выделения рамки)
    if (event->button() == Qt::LeftButton) {
        m_isZoomed = true;
    }

    QChartView::mousePressEvent(event);
}

void SensorChart::resetZoom() {
    m_isZoomed = false;
    chart()->zoomReset();
    // Сразу вызываем обновление порогов под стандартный масштаб
    updateThresholdPositions();
}