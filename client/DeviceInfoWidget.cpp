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
    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_intervalCombo = new QComboBox(this);
    m_intervalCombo->addItem("15 минут", 900);
    m_intervalCombo->addItem("1 час", 3600);
    m_intervalCombo->addItem("3 часа", 10800);
    m_intervalCombo->addItem("12 часов", 43200);
    m_intervalCombo->addItem("Сутки", 86400);
    m_intervalCombo->setCurrentIndex(1); // По умолчанию "1 час"

    m_infoLabel = new QLabel("Выберите устройство в дереве", this);
    m_infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    
    headerLayout->addWidget(m_infoLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel("Интервал:", this));
    headerLayout->addWidget(m_intervalCombo);

    mainLayout->addLayout(headerLayout);

    m_sensorsTable = new QTableWidget(0, 3, this);
    m_sensorsTable->setHorizontalHeaderLabels({ "Датчик", "Текущее", "Среднее" }); // Убрали "(за час)"
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setFixedHeight(150);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sensorsTable->setIconSize(QSize(16, 16));
    m_sensorsTable->setFocusPolicy(Qt::NoFocus);

    mainLayout->addWidget(m_sensorsTable);

    // Изменили текст на более универсальный
    mainLayout->addWidget(new QLabel("История показаний (по выбранному интервалу):", this));

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget();
    m_chartsLayout = new QVBoxLayout(container);
    m_chartsLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(container);

    mainLayout->addWidget(scrollArea);
 
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceInfoWidget::onIntervalChanged);
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

        // --- 1. Логика тренда (Сравнение с прошлым значением) ---
        double lastVal = 0.0;
        double prevVal = 0.0;
        bool hasTrend = false;

        QSqlQuery qTrend(DatabaseManager::instance().database());
        qTrend.prepare("SELECT value FROM sensor_data WHERE sensor_id = ? ORDER BY timestamp DESC LIMIT 2");
        qTrend.addBindValue(s.id);

        if (qTrend.exec()) {
            if (qTrend.next()) {
                lastVal = qTrend.value(0).toDouble();
                if (qTrend.next()) {
                    prevVal = qTrend.value(0).toDouble();
                    hasTrend = true;
                }
            }
        }

        // --- 2. Расчет среднего (ваш существующий код) ---
        double avgVal = 0.0;
        QSqlQuery qAvg(DatabaseManager::instance().database());
        qAvg.prepare("SELECT AVG(value) FROM sensor_data "
            "WHERE sensor_id = ? AND timestamp > DATE_SUB(NOW(), INTERVAL ? SECOND)");
        qAvg.addBindValue(s.id);
        qAvg.addBindValue(m_currentIntervalSeconds);
        if (qAvg.exec() && qAvg.next()) {
            avgVal = qAvg.value(0).toDouble();
        }

        // --- 3. Заполнение таблицы с иконками ---
        m_sensorsTable->setItem(row, 0, new QTableWidgetItem(s.key));

        // Колонка "Текущее" с иконкой тренда
        QTableWidgetItem* lastItem = new QTableWidgetItem(s.lastValue + " " + s.unit);
        if (hasTrend) {
            if (lastVal > prevVal) {
                lastItem->setIcon(QIcon(":/icons/green_up_arrow.png"));
            }
            else if (lastVal < prevVal) {
                lastItem->setIcon(QIcon(":/icons/red_down_arrow.png"));
            }
        }
        m_sensorsTable->setItem(row, 1, lastItem);

        m_sensorsTable->setItem(row, 2, new QTableWidgetItem(QString::number(avgVal, 'f', 2) + " " + s.unit));

        // Обновление графиков
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
    chartWidget->setUnit(unit);
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
        "WHERE sensor_id = ? AND timestamp > UNIX_TIMESTAMP(NOW()) - ? "
        "ORDER BY timestamp ASC");
    q.addBindValue(sensorId);
    q.addBindValue(m_currentIntervalSeconds); 

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

        m_sensorCharts[sensorId]->setXAxisRange(now.addSecs(-m_currentIntervalSeconds), now.addSecs(300));
        m_sensorCharts[sensorId]->setPoints(points);
    }
}

void DeviceInfoWidget::onIntervalChanged(int index) {
    // Получаем секунды из UserData элемента[cite: 6]
    m_currentIntervalSeconds = m_intervalCombo->itemData(index).toInt();

    // Сбрасываем ручной зум, чтобы оси перенастроились под новый интервал
    for (auto chart : m_sensorCharts.values()) {
        chart->resetZoom();
    }

    updateData(); // Перезагружаем таблицу и графики
}