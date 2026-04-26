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

    // Таблица: Датчик | Значение | Среднее (1ч)
    m_sensorsTable = new QTableWidget(0, 3, this);
    m_sensorsTable->setHorizontalHeaderLabels({ "Датчик", "Значение", "Среднее (за час)" });
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setFixedHeight(150);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_sensorsTable);

    mainLayout->addWidget(new QLabel("История за последний час:", this));

    // Настройка области прокрутки для графиков
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

void DeviceInfoWidget::clearCharts() {
    QLayoutItem* item;
    while ((item = m_chartsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    qDeleteAll(m_sensorCharts);
    m_sensorCharts.clear();
}

void DeviceInfoWidget::updateData() {
    if (m_currentDeviceId == -1) return;

    auto sensors = DatabaseManager::instance().getSensorsForDevice(m_currentDeviceId);
    m_sensorsTable->setRowCount(0);

    for (const auto& s : sensors) {
        int row = m_sensorsTable->rowCount();
        m_sensorsTable->insertRow(row);

        // 1. Считаем среднее значение за час средствами SQL
        double avgVal = 0.0;
        QSqlQuery qAvg(DatabaseManager::instance().database());
        qAvg.prepare("SELECT AVG(value) FROM sensor_data "
            "WHERE sensor_id = ? AND timestamp > NOW() - INTERVAL 1 HOUR");
        qAvg.addBindValue(s.id);
        if (qAvg.exec() && qAvg.next()) {
            avgVal = qAvg.value(0).toDouble();
        }

        // 2. Обновляем таблицу
        m_sensorsTable->setItem(row, 0, new QTableWidgetItem(s.key));
        m_sensorsTable->setItem(row, 1, new QTableWidgetItem(s.lastValue + " " + s.unit));
        m_sensorsTable->setItem(row, 2, new QTableWidgetItem(QString::number(avgVal, 'f', 2) + " " + s.unit));

        // 3. Работа с графиком
        if (!m_sensorCharts.contains(s.id)) {
            createSensorChart(s.id, s.key, s.unit);
        }

        // Обновляем точки на существующем или новом графике
        updateSensorChartData(s.id);
    }
}

void DeviceInfoWidget::createSensorChart(int sensorId, const QString& name, const QString& unit) {
    SensorChartBundle* bundle = new SensorChartBundle();
    bundle->series = new QLineSeries();
    QChart* chart = new QChart();
    
    chart->addSeries(bundle->series); // 1. Сначала серию
    chart->setTitle(name + " (" + unit + ")");
    chart->legend()->hide();

    bundle->axisX = new QDateTimeAxis();
    bundle->axisX->setFormat("HH:mm");
    bundle->axisX->setTitleText("Время");
    chart->addAxis(bundle->axisX, Qt::AlignBottom); // 2. Добавляем оси в чарт

    bundle->axisY = new QValueAxis();
    bundle->axisY->setTitleText(unit);
    chart->addAxis(bundle->axisY, Qt::AlignLeft);

    // 3. ПРИВЯЗЫВАЕМ серию к уже добавленным в чарт осям
    bundle->series->attachAxis(bundle->axisX); 
    bundle->series->attachAxis(bundle->axisY);

    bundle->view = new QChartView(chart);
    bundle->view->setRenderHint(QPainter::Antialiasing);
    bundle->view->setMinimumHeight(220);

    m_chartsLayout->addWidget(bundle->view);
    m_sensorCharts.insert(sensorId, bundle);
}
void DeviceInfoWidget::updateSensorChartData(int sensorId) {
    if (!m_sensorCharts.contains(sensorId)) return;
    auto* bundle = m_sensorCharts[sensorId];

    QSqlQuery q(DatabaseManager::instance().database());
    // Используем UNIX_TIMESTAMP для надежности преобразования
    q.prepare("SELECT value, UNIX_TIMESTAMP(timestamp) FROM sensor_data "
        "WHERE sensor_id = ? AND timestamp > NOW() - INTERVAL 1 HOUR "
        "ORDER BY timestamp ASC");
    q.addBindValue(sensorId);

    if (q.exec()) {
        QList<QPointF> points;
        double minY = 999999, maxY = -999999;

        while (q.next()) {
            double val = q.value(0).toDouble();
            // UNIX_TIMESTAMP возвращает секунды, QtCharts нужны миллисекунды
            qint64 msecs = q.value(1).toLongLong() * 1000;
            points.append(QPointF(msecs, val));

            if (val < minY) minY = val;
            if (val > maxY) maxY = val;
        }

        // ПРИНУДИТЕЛЬНО устанавливаем диапазон оси X на текущий час, 
        // даже если точек мало, чтобы не видеть "03:00"
        QDateTime now = QDateTime::currentDateTime();
        QDateTime start = now.addSecs(-3600);
        bundle->axisX->setRange(start, now);

        if (!points.isEmpty()) {
            bundle->series->replace(points);

            // Если точки выходят за пределы часа (редко, но бывает), расширяем
            if (QDateTime::fromMSecsSinceEpoch(points.first().x()) < start) {
                bundle->axisX->setMin(QDateTime::fromMSecsSinceEpoch(points.first().x()));
            }

            double padding = (maxY - minY) * 0.15;
            if (padding == 0) padding = 1.0;
            bundle->axisY->setRange(minY - padding, maxY + padding);
        }
        else {
            bundle->series->clear(); // Если данных нет, очищаем старую линию
        }
    }
}