#include "DeviceInfoWidget.h"
#include <QHeaderView>
#include <QDateTime>
#include <QSqlQuery>
#include <QDebug>

DeviceInfoWidget::DeviceInfoWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void DeviceInfoWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_infoLabel = new QLabel("Выберите устройство в дереве", this);
    m_infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(m_infoLabel);

    m_sensorsTable = new QTableWidget(0, 3, this);
    m_sensorsTable->setHorizontalHeaderLabels({ "Датчик", "Значение", "Среднее (за час)" });
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setFixedHeight(150);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_sensorsTable);

    mainLayout->addWidget(new QLabel("История за последний час:", this));

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget();
    m_chartsLayout = new QVBoxLayout(container);
    m_chartsLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(container);

    mainLayout->addWidget(scrollArea);
}

void DeviceInfoWidget::setDevice(int deviceId, const QString& name) {
    if (m_currentDeviceId == deviceId) return;

    m_currentDeviceId = deviceId;
    m_currentDeviceName = name;
    m_infoLabel->setText("Устройство: " + name);

    clearCharts(); // Очищаем старые графики при переключении устройства
    updateData();  // Первичное заполнение
}

void DeviceInfoWidget::updateData() {
    if (m_currentDeviceId == -1) return;

    auto sensors = DatabaseManager::instance().getSensorsForDevice(m_currentDeviceId);
    m_sensorsTable->setRowCount(0);

    for (const auto& s : sensors) {
        int row = m_sensorsTable->rowCount();
        m_sensorsTable->insertRow(row);

        double avgVal = 0.0;
        QSqlQuery qAvg(DatabaseManager::instance().database());
        qAvg.prepare("SELECT AVG(value) FROM sensor_data "
            "WHERE sensor_id = ? AND timestamp > NOW() - INTERVAL 1 HOUR");
        qAvg.addBindValue(s.id);
        if (qAvg.exec() && qAvg.next()) {
            avgVal = qAvg.value(0).toDouble();
        }

        m_sensorsTable->setItem(row, 0, new QTableWidgetItem(s.key));
        m_sensorsTable->setItem(row, 1, new QTableWidgetItem(s.lastValue + " " + s.unit));
        m_sensorsTable->setItem(row, 2, new QTableWidgetItem(QString::number(avgVal, 'f', 2) + " " + s.unit));

        if (!m_sensorCharts.contains(s.id)) {
            createSensorChart(s.id, s.key, s.unit);
        }

        updateSensorChartData(s.id);
    }
}

void DeviceInfoWidget::clearCharts() {
    QLayoutItem* item;
    while ((item = m_chartsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    // m_sensorCharts очистится автоматически при удалении виджетов из layout
    m_sensorCharts.clear();
}

void DeviceInfoWidget::createSensorChart(int sensorId, const QString& name, const QString& unit) {
    // Используем наш унифицированный компонент
    SensorChart* chartWidget = new SensorChart(name + " (" + unit + ")", sensorId, this);
    chartWidget->setMinimumHeight(220);

    // Загружаем пределы из БД и передаем в график[cite: 19, 20]
    SensorInfo info = DatabaseManager::instance().getSensorSettings(sensorId);
    chartWidget->setThresholds(info.minLimit, info.maxLimit);

    m_chartsLayout->addWidget(chartWidget);
    m_sensorCharts.insert(sensorId, chartWidget);
}

void DeviceInfoWidget::updateSensorChartData(int sensorId) {
    if (!m_sensorCharts.contains(sensorId)) return;
    SensorChart* chartWidget = m_sensorCharts[sensorId];

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("SELECT value, UNIX_TIMESTAMP(timestamp) FROM sensor_data "
        "WHERE sensor_id = ? AND timestamp > NOW() - INTERVAL 1 HOUR "
        "ORDER BY timestamp ASC");
    q.addBindValue(sensorId);

    if (q.exec()) {
        QList<QPointF> points;
        while (q.next()) {
            double val = q.value(0).toDouble();
            qint64 msecs = q.value(1).toLongLong() * 1000;
            points.append(QPointF(msecs, val));
        }

        QDateTime now = QDateTime::currentDateTime();

        if (!points.isEmpty()) {
            QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(points.last().x());

            // Если данных нет больше минуты, не показываем прогноз
            if (lastTime.secsTo(now) < 60) {
                double pred = DatabaseManager::instance().predictFutureValue(sensorId, 300, 10);
                if (!std::isnan(pred)) {
                    m_sensorCharts[sensorId]->updatePrediction(lastTime, points.last().y(), 300, pred);
                }
                else {
                    m_sensorCharts[sensorId]->clearPrediction(); 
                }
            }
            else {
                m_sensorCharts[sensorId]->clearPrediction(); 
            }
        }

        // Установка диапазона оси X (с запасом в будущее)
        m_sensorCharts[sensorId]->setXAxisRange(now.addSecs(-1800), now.addSecs(300));
        m_sensorCharts[sensorId]->setPoints(points);
    }
}