#include "SensorChart.h"
#include "DatabaseManager.h"

#include <QLegendMarker>
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
    m_axisX->setTickCount(7); // Будет 6 интервалов времени
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

    m_cursorLine = new QLineSeries();
    QPen cursorPen(Qt::gray);
    cursorPen.setWidth(1);
    cursorPen.setStyle(Qt::DotLine); // Пунктирная линия
    m_cursorLine->setPen(cursorPen);

    m_chart->addSeries(m_cursorLine);
    m_cursorLine->attachAxis(m_axisX);
    m_cursorLine->attachAxis(m_axisY);

    // Скрываем линию курсора из легенды, чтобы не мешалась
    m_chart->legend()->markers(m_cursorLine).at(0)->setVisible(false);

    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
    m_series->setPointsVisible(true);
    setMouseTracking(true);

    if (m_sensorId != -1) {
        connect(&DatabaseManager::instance(), &DatabaseManager::sensorThresholdsChanged,
            this, &SensorChart::onThresholdsUpdated);
    }

    setRubberBand(QChartView::RectangleRubberBand);
}

void SensorChart::setThresholds(double min, double max) {
    m_minLine->clear();
    m_maxLine->clear();

    m_minLimit = min;
    m_maxLimit = max;

    if (!std::isnan(min)) m_minLine->append(0, min); // Временные точки, обновятся в update
    if (!std::isnan(max)) m_maxLine->append(0, max);

    updateThresholdPositions();
}

void SensorChart::addPoint(const QDateTime& time, double value) {
    m_series->append(time.toMSecsSinceEpoch(), value);

    // Сдвигаем окно, только если новая точка реально вышла за ПРАВУЮ границу
    if (m_baseXEnd.isValid() && time > m_baseXEnd) {
        // Вычисляем, насколько точка ушла вперед
        qint64 diffMSecs = m_baseXEnd.msecsTo(time);

        // Сдвигаем обе границы на эту разницу, сохраняя ширину окна (интервал)
        m_baseXStart = m_baseXStart.addMSecs(diffMSecs);
        m_baseXEnd = m_baseXEnd.addMSecs(diffMSecs);

        if (!m_isZoomed) {
            m_axisX->setRange(m_baseXStart, m_baseXEnd);
            updateThresholdPositions();
        }
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
    m_baseXStart = start;
    m_baseXEnd = end;

    // Если пользователь сейчас в режиме зума, мы обновляем базовые границы в памяти,
    // но не трогаем саму ось, чтобы не "выбивать" его из просмотра.
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

    if (m_isMouseOver) {
        // Ищем ближайшую точку в новых данных к m_lastHoverPoint
        // или просто перерисовываем старый текст по текущей позиции курсора
        showTooltip(m_lastHoverPoint);
    }

    updateThresholdPositions();
}

void SensorChart::onThresholdsUpdated(int sensorId, double min, double max) {
    if (sensorId == m_sensorId) {
        setThresholds(min, max);
    }
}

void SensorChart::updatePrediction(const QDateTime& currentTime, double currentValue, int futureSeconds, double predictedValue) {
    if (!m_axisX) return;

    m_hasPrediction = true;
    m_predictedValue = predictedValue;
    m_predictionTime = currentTime.addSecs(futureSeconds);

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
        m_hasPrediction = false;
        m_predictSeries->clear();
    }
}

void SensorChart::wheelEvent(QWheelEvent* event) {
    m_isZoomed = true;

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
        resetZoom(); // Сброс масштаба
        event->accept();
        return;
    }

    // Если была нажата левая кнопка (начало выделения рамки)
    if (event->button() == Qt::LeftButton) {
        m_isZoomed = true;
    }

    QChartView::mousePressEvent(event);
}
void SensorChart::resetZoom() {
    m_isZoomed = false;

    // ВАЖНО: Убираем chart()->zoomReset()!
    // Вместо этого принудительно ставим ось в границы, которые хранятся в памяти.
    if (m_baseXStart.isValid() && m_baseXEnd.isValid()) {
        m_axisX->setRange(m_baseXStart, m_baseXEnd);
    }

    // Опционально: делаем автозахват по Y, чтобы график не был "сплюснутым"
    QList<QPointF> points = m_series->points();
    if (!points.isEmpty()) {
        double minY = points.first().y();
        double maxY = points.first().y();
        for (const auto& p : points) {
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        double padding = (maxY - minY) * 0.15;
        if (padding == 0) padding = 1.0;
        m_axisY->setRange(minY - padding, maxY + padding);
    }

    updateThresholdPositions();
}

void SensorChart::setUnit(const QString& unit) {
    m_unit = unit;
}

void SensorChart::showTooltip(const QPointF& point) {
    qDebug() << "show";
    QString dt = QDateTime::fromMSecsSinceEpoch(point.x()).toString("HH:mm:ss");
    QString text = QString("<b>Время:</b> %1<br>"
        "<b>Значение:</b> %2 %3<br>"
        "<b>Пределы:</b> %4...%5 %3")
        .arg(dt).arg(point.y()).arg(m_unit).arg(m_minLimit).arg(m_maxLimit);

    if (m_hasPrediction) {
        text += QString("<br><hr><b>Прогноз (%1):</b><br><font color='magenta'>%2 %3</font>")
            .arg(m_predictionTime.toString("HH:mm:ss"))
            .arg(m_predictedValue, 0, 'f', 2).arg(m_unit);
    }

    // Используем глобальные координаты с отступом
    QPoint pos = QCursor::pos();
    pos.setX(pos.x() + 15);
    pos.setY(pos.y() - 15); // Чуть выше курсора, чтобы не перекрывать линию

    QToolTip::showText(pos, text, this, rect(), 5000);
}

void SensorChart::mouseMoveEvent(QMouseEvent* event) {
    QChartView::mouseMoveEvent(event); // Важно для зума

    if (!chart()->plotArea().contains(event->pos())) {
        m_cursorLine->setVisible(false);
        return;
    }

    QPointF valueAtMouse = chart()->mapToValue(event->pos(), m_series);
    const QList<QPointF> points = m_series->points();
    if (points.isEmpty()) return;

    auto it = std::lower_bound(points.begin(), points.end(), QPointF(valueAtMouse.x(), 0),
        [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });

    double targetX;
    if (it == points.begin()) targetX = it->x();
    else if (it == points.end()) targetX = (it - 1)->x();
    else {
        targetX = (qAbs((it - 1)->x() - valueAtMouse.x()) < qAbs(it->x() - valueAtMouse.x())) ? (it - 1)->x() : it->x();
    }

    m_cursorLine->replace({ QPointF(targetX, m_axisY->min()), QPointF(targetX, m_axisY->max()) });
    m_cursorLine->setVisible(true);
}

// В .h файле: void leaveEvent(QEvent* event) override;
void SensorChart::leaveEvent(QEvent* event) {
    qDebug() << "hide";
    m_cursorLine->setVisible(false);
    QToolTip::hideText();
    QChartView::leaveEvent(event);
}

bool SensorChart::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        const QList<QPointF> points = m_series->points();

        // Проверяем, что мышь внутри области графика
        if (points.isEmpty() || !chart()->plotArea().contains(helpEvent->pos())) {
            QToolTip::hideText();
            return true;
        }

        // 1. Точное преобразование координат
        // Используем mapToValue именно для этой позиции
        QPointF valueAtMouse = chart()->mapToValue(helpEvent->pos(), m_series);
        double mouseX = valueAtMouse.x();

        // 2. Поиск ближайшей точки
        auto it = std::lower_bound(points.begin(), points.end(), QPointF(mouseX, 0),
            [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });

        QPointF closestPoint;
        if (it == points.begin()) closestPoint = *it;
        else if (it == points.end()) closestPoint = *(it - 1);
        else {
            // Выбираем ту, что ближе по X математически
            closestPoint = (qAbs((it - 1)->x() - mouseX) < qAbs(it->x() - mouseX)) ? *(it - 1) : *it;
        }

        // 3. Формирование текста с "квадратиками" (цвета берем из серий)
        QString seriesColor = m_series->pen().color().name();
        QString minColor = m_minLine->pen().color().name();
        QString maxColor = m_maxLine->pen().color().name();

        QString dt = QDateTime::fromMSecsSinceEpoch(closestPoint.x()).toString("HH:mm:ss");

        QString text = QString(
            "<span style='color:%1;'>■</span> <b>Время:</b> %2<br>"
            "<span style='color:%1;'>■</span> <b>Значение:</b> %3 %4<br>"
            "<hr>"
            "<span style='color:%5;'>■</span> <b>Мин. порог:</b> %6 %4<br>"
            "<span style='color:%7;'>■</span> <b>Макс. порог:</b> %8 %4")
            .arg(seriesColor).arg(dt).arg(closestPoint.y()).arg(m_unit)
            .arg(minColor).arg(m_minLimit)
            .arg(maxColor).arg(m_maxLimit);

        // Добавляем прогноз, если он есть
        if (m_hasPrediction) {
            QString predColor = m_predictSeries->pen().color().name();
            text += QString("<br><span style='color:%1;'>■</span> <b>Прогноз (%2):</b> <font color='%1'>%3 %4</font>")
                .arg(predColor)
                .arg(m_predictionTime.toString("HH:mm:ss"))
                .arg(m_predictedValue, 0, 'f', 2).arg(m_unit);
        }

        QToolTip::showText(helpEvent->globalPos(), text, this);
        return true;
    }
    return QChartView::event(event);
}