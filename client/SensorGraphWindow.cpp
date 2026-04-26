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

    // --- Series & chart setup -------------------------------------------------
    m_series = new QLineSeries();
    m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->setTitle("Динамика изменения параметра");
    m_chart->legend()->hide();

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

    // --- Axes ---------------------------------------------------------------
    QDateTimeAxis* axisX = new QDateTimeAxis;
    axisX->setFormat("hh:mm:ss");
    axisX->setTitleText(tr("Время"));
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis;
    axisY->setTitleText(tr("Значение"));
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    // --- Chart view ----------------------------------------------------------
    QChartView* chartView = new QChartView(m_chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);

    // --- Main layout ----------------------------------------------------------
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(intervalLayout);
    mainLayout->addWidget(chartView);

    // --- Timer for live updates --------------------------------------------
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SensorGraphWindow::refreshData);
    m_timer->start(2000);

	//Load the default interval data
    applyInterval();
}

// Load data for the currently selected interval (called from applyInterval)
void SensorGraphWindow::loadData() {
    if (!DatabaseManager::instance().open()) return;

    QSqlQuery q(DatabaseManager::instance().database());
    const QDateTime start = m_startEdit->dateTime();
    const QDateTime end = m_endEdit->dateTime();

    q.prepare("SELECT value, timestamp FROM sensor_data WHERE sensor_id = ? AND timestamp BETWEEN ? AND ? ORDER BY timestamp ASC");
    q.addBindValue(m_sensorId);
    q.addBindValue(start);
    q.addBindValue(end);

    // Clear any existing points before loading a new range
    m_series->clear();
    m_lastTimestamp = QDateTime();

    if (q.exec()) {
        while (q.next()) {
            double val = q.value(0).toDouble();
            QDateTime dt = q.value(1).toDateTime();
            m_series->append(dt.toMSecsSinceEpoch(), val);
            m_lastTimestamp = dt; // keep track of newest point
        }
    }
    if (m_lastTimestamp.isNull()) {
        m_lastTimestamp = QDateTime::currentDateTime();
    }

    // Adjust axis ranges after loading
    if (!m_series->points().isEmpty()) {
        auto points = m_series->points();
        qint64 minX = points.front().x();
        qint64 maxX = points.back().x();
        auto axesX = m_chart->axes(Qt::Horizontal);
        auto axesY = m_chart->axes(Qt::Vertical);
        if (!axesX.isEmpty()) {
            QDateTimeAxis* ax = qobject_cast<QDateTimeAxis*>(axesX.first());
            if (ax) ax->setRange(QDateTime::fromMSecsSinceEpoch(minX), QDateTime::fromMSecsSinceEpoch(maxX));
        }
        if (!axesY.isEmpty()) {
            QValueAxis* ay = qobject_cast<QValueAxis*>(axesY.first());
            if (ay) {
                double minY = points.front().y();
                double maxY = points.front().y();
                for (const QPointF& p : points) {
                    if (p.y() < minY) minY = p.y();
                    if (p.y() > maxY) maxY = p.y();
                }
                double padding = (maxY - minY) * 0.1;
                if (padding == 0) padding = 1.0;
                ay->setRange(minY - padding, maxY + padding);
            }
        }
    }
}

// Slot invoked when the user clicks the Apply button
void SensorGraphWindow::applyInterval() {
    loadData(); // reload data for the newly chosen interval
}

void SensorGraphWindow::refreshData() {
    // Pull any rows newer than the last known timestamp
    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("SELECT value, timestamp FROM sensor_data WHERE sensor_id = ? AND timestamp > ? ORDER BY timestamp ASC");
    q.addBindValue(m_sensorId);
    q.addBindValue(m_lastTimestamp);

    if (q.exec()) {
        bool added = false;
        const QDateTime start = m_startEdit->dateTime();
        const QDateTime end = m_endEdit->dateTime();
        while (q.next()) {
            double val = q.value(0).toDouble();
            QDateTime dt = q.value(1).toDateTime();
            // Only add if within the user‑selected interval
            if (dt >= start && dt <= end) {
                m_series->append(dt.toMSecsSinceEpoch(), val);
                added = true;
            }
            // Update the newest timestamp regardless of interval (so future queries stay incremental)
            m_lastTimestamp = dt;
        }
        if (added) {
            // Update axis ranges similar to loadData()
            if (!m_series->points().isEmpty()) {
                auto points = m_series->points();
                qint64 minX = points.front().x();
                qint64 maxX = points.back().x();
                auto axesX = m_chart->axes(Qt::Horizontal);
                auto axesY = m_chart->axes(Qt::Vertical);
                if (!axesX.isEmpty()) {
                    QDateTimeAxis* ax = qobject_cast<QDateTimeAxis*>(axesX.first());
                    if (ax) ax->setRange(QDateTime::fromMSecsSinceEpoch(minX), QDateTime::fromMSecsSinceEpoch(maxX));
                }
                if (!axesY.isEmpty()) {
                    QValueAxis* ay = qobject_cast<QValueAxis*>(axesY.first());
                    if (ay) {
                        double minY = points.front().y();
                        double maxY = points.front().y();
                        for (const QPointF& p : points) {
                            if (p.y() < minY) minY = p.y();
                            if (p.y() > maxY) maxY = p.y();
                        }
                        double padding = (maxY - minY) * 0.1;
                        if (padding == 0) padding = 1.0;
                        ay->setRange(minY - padding, maxY + padding);
                    }
                }
            }
        }
    } else {
        qDebug() << "Ошибка при загрузке новых точек графика:" << q.lastError();
    }
}

void SensorGraphWindow::appendPoint(double value, const QDateTime& timestamp)
{
    const QDateTime start = m_startEdit->dateTime();
    const QDateTime end = m_endEdit->dateTime();

    bool within = (timestamp >= start && timestamp <= end);
    if (within) {
        m_series->append(timestamp.toMSecsSinceEpoch(), value);
    }
    // Update m_lastTimestamp if this timestamp is newer
    if (timestamp > m_lastTimestamp) {
        m_lastTimestamp = timestamp;
    }

    if (within && !m_series->points().isEmpty()) {
        // Update axis ranges
        auto points = m_series->points();
        qint64 minX = points.front().x();
        qint64 maxX = points.back().x();
        auto axesX = m_chart->axes(Qt::Horizontal);
        auto axesY = m_chart->axes(Qt::Vertical);
        if (!axesX.isEmpty()) {
            QDateTimeAxis* ax = qobject_cast<QDateTimeAxis*>(axesX.first());
            if (ax) ax->setRange(QDateTime::fromMSecsSinceEpoch(minX), QDateTime::fromMSecsSinceEpoch(maxX));
        }
        if (!axesY.isEmpty()) {
            QValueAxis* ay = qobject_cast<QValueAxis*>(axesY.first());
            if (ay) {
                double minY = points.front().y();
                double maxY = points.front().y();
                for (const QPointF& p : points) {
                    if (p.y() < minY) minY = p.y();
                    if (p.y() > maxY) maxY = p.y();
                }
                double padding = (maxY - minY) * 0.1;
                if (padding == 0) padding = 1.0;
                ay->setRange(minY - padding, maxY + padding);
            }
        }
    }

}

SensorGraphWindow::~SensorGraphWindow() {
    if (m_timer) {
        m_timer->stop();
        disconnect(m_timer, &QTimer::timeout, this, &SensorGraphWindow::refreshData);
    }
}