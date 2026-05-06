#include "SensorGraphWindow.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QSqlQuery>
#include <QDebug>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

SensorGraphWindow::SensorGraphWindow(int sensorId, const QString& sensorName, QWidget* parent)
    : QDialog(parent), m_sensorId(sensorId) {

    setWindowTitle("История показаний: " + sensorName);
    resize(800, 500);

    // --- Time‑interval UI ----------------------------------------------------
    m_startEdit = new QDateTimeEdit(this);
    m_endEdit = new QDateTimeEdit(this);
    m_applyBtn = new QPushButton(tr("Применить"), this);

    // Default interval: last 24 h
    QDateTime now = QDateTime::currentDateTime();
    m_endEdit->setDateTime(now);
    m_startEdit->setDateTime(now.addDays(-1));
    m_startEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_endEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    connect(m_applyBtn, &QPushButton::clicked, this, &SensorGraphWindow::applyInterval);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    intervalLayout->addWidget(new QLabel(tr("Начало:"), this));
    intervalLayout->addWidget(m_startEdit);
    intervalLayout->addWidget(new QLabel(tr("Конец:"), this));
    intervalLayout->addWidget(m_endEdit);
    intervalLayout->addWidget(m_applyBtn);

    m_sensorChart = new SensorChart("История показаний", sensorId, this);

    SensorInfo info = DatabaseManager::instance().getSensorSettings(m_sensorId);
    m_sensorChart->setThresholds(info.minLimit, info.maxLimit);
    // --- Main layout ----------------------------------------------------------
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(intervalLayout);
    mainLayout->addWidget(m_sensorChart);

    // --- Timer for live updates --------------------------------------------
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SensorGraphWindow::loadData);
    m_timer->start(2000);

    m_lastTimestamp.setTimeZone(QTimeZone::LocalTime);

	//Load the default interval data
    applyInterval();
}

void SensorGraphWindow::loadData() {
    if (!DatabaseManager::instance().open()) return;

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("SELECT value, timestamp FROM sensor_data "
        "WHERE sensor_id = ? AND timestamp BETWEEN ? AND ? "
        "ORDER BY timestamp ASC");
    q.addBindValue(m_sensorId);
    q.addBindValue(m_start.toString("yyyy-MM-dd HH:mm:ss"));
    q.addBindValue(m_end.toString("yyyy-MM-dd HH:mm:ss"));

    if (q.exec()) {
        QList<QPointF> points;
        QDateTime lastDt;

        while (q.next()) {
            double val = q.value(0).toDouble();
            QDateTime dt = q.value(1).toDateTime();
            dt.setTimeZone(QTimeZone::LocalTime);

            // Собираем точки в список
            points.append(QPointF(dt.toMSecsSinceEpoch(), val));
            lastDt = dt;
        }

        // Передаем данные в наш компонент[cite: 30, 31]
        m_sensorChart->setXAxisRange(m_start, m_end);
        m_sensorChart->setPoints(points);

        // Работа с прогнозом (теперь m_series не нужен)
        if (!points.isEmpty()) {
            double predicted = DatabaseManager::instance().predictFutureValue(m_sensorId, 300, 10);
            if (!std::isnan(predicted)) {
                m_sensorChart->updatePrediction(lastDt, points.last().y(), 300, predicted);
            }
            else {
                m_sensorChart->clearPrediction();
            }
        }
    }
}

// Slot invoked when the user clicks the Apply button
void SensorGraphWindow::applyInterval() {
    m_start = m_startEdit->dateTime();
    m_end = m_endEdit->dateTime();
    loadData();
}

SensorGraphWindow::~SensorGraphWindow() {
    if (m_timer) {
        m_timer->stop();
        disconnect(m_timer, &QTimer::timeout, this, &SensorGraphWindow::loadData);
    }
}

void SensorGraphWindow::appendPoint(double value, const QDateTime& time) {
    // Просто вызываем метод нашего кастомного графика[cite: 30, 31]
    m_sensorChart->addPoint(time, value);

    // По желанию: здесь же можно обновлять линию прогноза при каждом новом сообщении
    double predicted = DatabaseManager::instance().predictFutureValue(m_sensorId, 300, 10);
    if (!std::isnan(predicted)) {
        m_sensorChart->updatePrediction(time, value, 300, predicted);
    }
}