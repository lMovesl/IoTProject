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

    // 1. Верхняя панель управления
    m_intervalCombo = new QComboBox(this);
    m_intervalCombo->addItem("15 минут", 900);
    m_intervalCombo->addItem("1 час", 3600);
    m_intervalCombo->addItem("3 часа", 10800);
    m_intervalCombo->addItem("12 часов", 43200);
    m_intervalCombo->addItem("Сутки", 86400);
    m_intervalCombo->setCurrentIndex(1);

    m_infoLabel = new QLabel("Выберите устройство в дереве", this);
    m_infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");

    headerLayout->addWidget(m_infoLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel("Интервал:", this));
    headerLayout->addWidget(m_intervalCombo);
    mainLayout->addLayout(headerLayout);

    // 2. Настройка Dashboard (Центральная зона)
    m_dashboard = new QMainWindow(this);
    m_dashboard->setWindowFlags(Qt::Widget);

    // Включаем все продвинутые возможности стыковки
    m_dashboard->setDockOptions(QMainWindow::AllowNestedDocks |
        QMainWindow::AllowTabbedDocks |
        QMainWindow::AnimatedDocks);
    m_dashboard->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);

    // Заглушка для центрального виджета (обязательна для QMainWindow)
    QWidget* dummyCentral = new QWidget();
    dummyCentral->setFixedSize(0, 0);
    m_dashboard->setCentralWidget(dummyCentral);

    mainLayout->addWidget(m_dashboard, 1);

    // 3. Создание Таймлайна (Статус сети)
    m_timelineDock = new QDockWidget("Статус сети", this);
    m_timelineDock->setObjectName("TimelineDock");
    m_timelineDock->setAllowedAreas(Qt::AllDockWidgetAreas); // Разрешаем крепить везде

    m_timelineWidget = new UptimeTimelineWidget(m_timelineDock);
    m_timelineDock->setWidget(m_timelineWidget);
    m_dashboard->addDockWidget(Qt::TopDockWidgetArea, m_timelineDock);

    // 4. Создание Таблицы (Текущие показатели)
    m_tableDock = new QDockWidget("Текущие показатели", this);
    m_tableDock->setObjectName("TableDock");
    m_tableDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_sensorsTable = new QTableWidget(0, 3, m_tableDock);
    m_sensorsTable->setHorizontalHeaderLabels({ "Датчик", "Текущее", "Среднее" });
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sensorsTable->setFocusPolicy(Qt::NoFocus);

    m_tableDock->setWidget(m_sensorsTable);

    // Добавляем таблицу слева (она займет место рядом с таймлайном)
    m_dashboard->addDockWidget(Qt::LeftDockWidgetArea, m_tableDock);

    // Связываем сигнал изменения интервала
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &DeviceInfoWidget::onIntervalChanged);
}

void DeviceInfoWidget::setDevice(int deviceId, const QString& name) {
    clearCharts(); // Здесь сохранится старое состояние

    m_currentDeviceId = deviceId;
    m_currentDeviceName = name;
    m_infoLabel->setText("Устройство: " + name);

    auto sensors = DatabaseManager::instance().getSensorsForDevice(deviceId);
    for (const auto& s : sensors) {
        createSensorChart(s.id, s.key, s.unit);
    }

    // 2. Восстанавливаем геометрию (вкладки, привязки)
    if (!m_dashboardState.isEmpty()) {
        m_dashboard->restoreState(m_dashboardState);
    }

    updateData();
}

void DeviceInfoWidget::updateData() {
    if (m_currentDeviceId == -1) return;

    qint64 end = QDateTime::currentSecsSinceEpoch();
    qint64 start = end - m_currentIntervalSeconds;

    QVector<DeviceStateInterval> history = fetchDeviceUptime(m_currentDeviceId, start, end);

    m_timelineWidget->setDeviceName("Время в сети");
    m_timelineWidget->setData(history, start, end);

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
    // 1. Сохраняем, как всё стояло
    if (!m_chartDocks.isEmpty()) {
        m_dashboardState = m_dashboard->saveState();
    }

    for (QDockWidget* dock : m_chartDocks.values()) {
        m_dashboard->removeDockWidget(dock);
        delete dock;
    }
    m_chartDocks.clear();
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

    QDockWidget* chartDock = new QDockWidget(name, this);
    int chartIndex = m_sensorCharts.size();
    chartDock->setObjectName(QString("ChartDock_%1").arg(chartIndex));
    chartDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    chartDock->setFeatures(QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable);
    chartDock->setWidget(chartWidget);

    // Добавляем график в нижнюю зону
    m_dashboard->addDockWidget(Qt::BottomDockWidgetArea, chartDock);

    m_sensorCharts.insert(sensorId, chartWidget);
    m_chartDocks.insert(sensorId, chartDock);
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

QVector<DeviceStateInterval> DeviceInfoWidget::fetchDeviceUptime(int deviceId, qint64 rangeStart, qint64 rangeEnd) {
    QVector<DeviceStateInterval> intervals;
    QSqlQuery q(DatabaseManager::instance().database());

    // Получаем ВСЕ отметки времени от ВСЕХ датчиков этого устройства за период
    q.prepare("SELECT UNIX_TIMESTAMP(timestamp) FROM sensor_data "
        "WHERE sensor_id IN (SELECT id FROM sensors WHERE device_id = ?) "
        "AND timestamp BETWEEN FROM_UNIXTIME(?) AND FROM_UNIXTIME(?) "
        "ORDER BY timestamp ASC");
    q.addBindValue(deviceId);
    q.addBindValue(rangeStart);
    q.addBindValue(rangeEnd);

    if (!q.exec()) return intervals;

    const qint64 TIMEOUT = 300; // 5 минут. Если данных нет дольше - считаем оффлайном
    qint64 currentStart = -1;
    qint64 prevTime = -1;

    while (q.next()) {
        qint64 time = q.value(0).toLongLong();

        if (currentStart == -1) {
            // Самая первая точка в периоде
            currentStart = time;
            // Если первая точка появилась намного позже начала графика, значит сначала был оффлайн
            if (currentStart - rangeStart > TIMEOUT) {
                intervals.append({ rangeStart, currentStart, false }); // Красный
            }
        }
        else {
            // Проверяем разницу с предыдущим пакетом
            if (time - prevTime > TIMEOUT) {
                // Был обрыв связи! 
                // 1. Закрываем зеленую зону (от старта до момента обрыва)
                intervals.append({ currentStart, prevTime, true });
                // 2. Добавляем красную зону (время, пока не было пакетов)
                intervals.append({ prevTime, time, false });
                // 3. Начинаем новую зеленую зону с текущей точки
                currentStart = time;
            }
        }
        prevTime = time;
    }

    // Закрываем последний кусок до правого края графика
    if (currentStart != -1) {
        if (rangeEnd - prevTime > TIMEOUT) {
            // Устройство отвалилось до конца периода
            intervals.append({ currentStart, prevTime, true });
            intervals.append({ prevTime, rangeEnd, false });
        }
        else {
            // Устройство стабильно работало до текущего момента
            intervals.append({ currentStart, rangeEnd, true });
        }
    }
    else {
        // Если база пустая за этот период - рисуем сплошную красную линию
        intervals.append({ rangeStart, rangeEnd, false });
    }

    return intervals;
}