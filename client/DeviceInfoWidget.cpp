#include "DeviceInfoWidget.h"
#include <QHeaderView>
#include <QDateTime>
#include <QSqlQuery>
#include <QDebug>
#include <QProgressBar>



DeviceInfoWidget::DeviceInfoWidget(QWidget* parent) : QWidget(parent) {
    m_statsUpdateTimer.start();
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

    // 2. Настройка Dashboard (Центральная зона на базе QMainWindow)
    m_dashboard = new QMainWindow(this);
    m_dashboard->setStyleSheet(
        // 1. Линии-разделители между панелями (когда они пристыкованы)
        "QMainWindow::separator {"
        "    background: #bdc3c7;"
        "    width: 2px; "
        "    height: 2px;"
        "}"
        // 2. Рамка вокруг самой панели
        "QDockWidget {"
        "    border: 1px solid #bdc3c7;"
        "}"
        // 3. Оформление заголовка и линии под ним
        "QDockWidget::title {"
        "    background: #f8f9fa;"
        "    text-align: left;"
        "    padding-left: 10px;"
        "    border-bottom: 1px solid #bdc3c7;" // Линия, отделяющая заголовок от графика
        "}"
        // 4. Белый фон и рамка для виджета внутри панели
        "QDockWidget > QWidget {"
        "    border: 1px solid #dcdde1;"
        "    background: white;"
        "}"
    );
    m_dashboard->setWindowFlags(Qt::Widget);
    m_dashboard->setDockOptions(QMainWindow::AllowNestedDocks |
        QMainWindow::AllowTabbedDocks |
        QMainWindow::AnimatedDocks);
    m_dashboard->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);

    QWidget* dummyCentral = new QWidget();
    dummyCentral->setFixedSize(0, 0); // Скрываем центральный виджет, чтобы доки заняли всё место
    m_dashboard->setCentralWidget(dummyCentral);

    mainLayout->addWidget(m_dashboard, 1);

    // 3. Создание Таймлайна (Статус сети)
    m_timelineDock = new QDockWidget("Статус сети", this);
    m_timelineDock->setObjectName("TimelineDock");
    m_timelineWidget = new UptimeTimelineWidget(m_timelineDock);
    m_timelineDock->setWidget(m_timelineWidget);
    m_timelineDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_timelineDock->setFeatures(QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable);
    m_dashboard->addDockWidget(Qt::TopDockWidgetArea, m_timelineDock);

    // 4. Создание Таблицы (Текущие показатели)
    m_tableDock = new QDockWidget("Текущие показатели", this);
    m_tableDock->setObjectName("TableDock");
    m_sensorsTable = new QTableWidget(0, 3, m_tableDock);
    m_sensorsTable->setHorizontalHeaderLabels({ "Датчик", "Текущее", "Среднее" });
    m_sensorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_tableDock->setWidget(m_sensorsTable);
    m_tableDock->setFeatures(QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable);
    m_dashboard->setCentralWidget(m_tableDock);
    //m_dashboard->addDockWidget(Qt::LeftDockWidgetArea, m_tableDock);

    // 5. Создание Аналитики (Статистика) с прокруткой
    m_statsDock = new QDockWidget("Аналитика: Статистика", this);
    m_statsDock->setObjectName("StatsDock");
    m_statsDock->setMinimumWidth(260);
    m_statsDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_statsDock->setFeatures(QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable);

    QScrollArea* statsScroll = new QScrollArea(m_statsDock);
    statsScroll->setWidgetResizable(true);
    statsScroll->setFrameShape(QFrame::NoFrame);

    m_statsContentWidget = new QWidget();
    m_statsMainLayout = new QVBoxLayout(m_statsContentWidget);
    m_statsMainLayout->setAlignment(Qt::AlignTop); // Прижимаем аналитику вверх

    statsScroll->setWidget(m_statsContentWidget);
    m_statsDock->setWidget(statsScroll);
    m_dashboard->addDockWidget(Qt::LeftDockWidgetArea, m_statsDock);

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
    m_statsUpdateTimer.start();
    updateStatistics(); 

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
    if (!m_statsUpdateTimer.isValid() || m_statsUpdateTimer.hasExpired(5000)) {
        updateStatistics();
        m_statsUpdateTimer.restart(); // Сбрасываем счетчик
    }
}

int DeviceInfoWidget::getCurrentIDDevice() const
{
    return m_currentDeviceId;
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

    if (m_chartDocks.isEmpty()) {
        // Если это первый график — просто добавляем его вниз
        m_dashboard->addDockWidget(Qt::BottomDockWidgetArea, chartDock);
    }
    else {
        // Если графики уже есть — берем последний созданный и «наслаиваем» новый на него
        QDockWidget* lastDock = m_chartDocks.values().last();
        m_dashboard->tabifyDockWidget(lastDock, chartDock);

        // Опционально: делаем новый созданный график активной вкладкой
        chartDock->raise();
    }

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

    updateData();
    updateStatistics();
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

QWidget* DeviceInfoWidget::createMetricRow(const QString& name, double min, double max, double avg, double stdDev, const QString& unit) {
    QWidget* row = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(row);
    layout->setContentsMargins(5, 5, 5, 10);
    layout->setSpacing(2);

    // 1. Заголовок датчика
    QLabel* nameLabel = new QLabel(QString("<b>%1</b>").arg(name));
    layout->addWidget(nameLabel);

    // 2. Слой с основными числами
    QHBoxLayout* valuesLayout = new QHBoxLayout();
    auto addVal = [&](const QString& label, double val, const QString& color) {
        QLabel* l = new QLabel(QString("<span style='color:%3'>%1:</span> <b>%2</b>")
            .arg(label).arg(QString::number(val, 'f', 1)).arg(color));
        l->setStyleSheet("font-size: 11px;");
        valuesLayout->addWidget(l);
        };

    addVal("Мин", min, "#2980b9");
    addVal("Ср", avg, "#27ae60");
    addVal("Макс", max, "#c0392b");
    valuesLayout->addStretch();

    // Добавляем сигму (отклонение) отдельно
    QLabel* sigmaLabel = new QLabel(QString("σ: ±%1").arg(QString::number(stdDev, 'f', 2)));
    sigmaLabel->setStyleSheet("color: #7f8c8d; font-size: 11px; font-style: italic;");
    valuesLayout->addWidget(sigmaLabel);

    layout->addLayout(valuesLayout);

    int progressPercent = 0;
    if (max > min) {
        progressPercent = static_cast<int>(((avg - min) / (max - min)) * 100);
    }

    QProgressBar* rangeBar = new QProgressBar();
    rangeBar->setTextVisible(false);
    rangeBar->setFixedHeight(6);

    rangeBar->setStyleSheet(
        "QProgressBar { "
        "   background: #e0e0e0; "
        "   border: none; "
        "   border-radius: 3px; "
        "}"
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3498db, stop:1 #2ecc71); "
        "   border-radius: 3px; "
        "}"
    );

    rangeBar->setRange(0, 100);
    rangeBar->setValue(progressPercent); 

    layout->addWidget(rangeBar);
    return row;
}

void DeviceInfoWidget::updateStatistics() {
    if (m_currentDeviceId == -1) return;

    auto sensors = DatabaseManager::instance().getSensorsForDevice(m_currentDeviceId);
    QSet<int> activeSensors;

    for (const auto& s : sensors) {
        activeSensors.insert(s.id);

        QSqlQuery q(DatabaseManager::instance().database());
        // Получаем все нужные данные одним запросом к БД
        q.prepare("SELECT MIN(value), MAX(value), AVG(value), STDDEV(value) "
            "FROM sensor_data "
            "WHERE sensor_id = ? AND timestamp > DATE_SUB(NOW(), INTERVAL ? SECOND)");
        q.addBindValue(s.id);
        q.addBindValue(m_currentIntervalSeconds);

        if (q.exec() && q.next()) {
            // Если данных за период нет, пропускаем
            if (q.value(0).isNull()) continue;

            double minV = q.value(0).toDouble();
            double maxV = q.value(1).toDouble();
            double avg = q.value(2).toDouble();
            double stdDev = q.value(3).toDouble();

            if (!m_sensorRows.contains(s.id)) {
                m_sensorRows[s.id] = createInteractiveMetricRow(s.key, s.unit);
                m_statsMainLayout->insertWidget(m_statsMainLayout->count() - 1, m_sensorRows[s.id].container);
            }

            SensorStatRow& row = m_sensorRows[s.id];

            // Теперь 'avg' здесь будет идентичен 'avgVal' из метода updateData()
            row.valuesLabel->setText(QString(
                "<span style='color:#2980b9'>Мин:</span> <b>%1</b>  "
                "<span style='color:#27ae60'>Ср:</span> <b>%2</b>  "
                "<span style='color:#c0392b'>Макс:</span> <b>%3</b>")
                .arg(QString::number(minV, 'f', 1))
                .arg(QString::number(avg, 'f', 1))
                .arg(QString::number(maxV, 'f', 1)));

            row.sigmaLabel->setText(QString("σ: ±%1").arg(QString::number(stdDev, 'f', 2)));
            row.minScaleLabel->setText(QString::number(minV, 'f', 1));
            row.maxScaleLabel->setText(QString::number(maxV, 'f', 1));
            row.avgScaleLabel->setText(QString::number(avg, 'f', 1));

            int progressPercent = 0;
            if (maxV > minV) {
                progressPercent = static_cast<int>(((avg - minV) / (maxV - minV)) * 100);
            }
            row.rangeBar->setValue(progressPercent);
        }
    }

    auto it = m_sensorRows.begin();
    while (it != m_sensorRows.end()) {
        if (!activeSensors.contains(it.key())) {
            it.value().container->deleteLater();
            it = m_sensorRows.erase(it);
        }
        else {
            ++it;
        }
    }
}

SensorStatRow DeviceInfoWidget::createInteractiveMetricRow(const QString& name, const QString& unit) {
    SensorStatRow row;
    row.container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(row.container);
    layout->setContentsMargins(5, 5, 5, 10);
    layout->setSpacing(0); // Уменьшаем отступы для компактности

    // 1. Заголовок и основные числа (как было)
    QLabel* nameLabel = new QLabel(QString("<b>%1</b> (%2)").arg(name, unit));
    layout->addWidget(nameLabel);

    QHBoxLayout* valuesLayout = new QHBoxLayout();
    row.valuesLabel = new QLabel();
    row.valuesLabel->setStyleSheet("font-size: 11px;");
    row.sigmaLabel = new QLabel();
    row.sigmaLabel->setStyleSheet("color: #7f8c8d; font-size: 10px; font-style: italic;");
    valuesLayout->addWidget(row.valuesLabel);
    valuesLayout->addStretch();
    valuesLayout->addWidget(row.sigmaLabel);
    layout->addLayout(valuesLayout);

    // 2. Прогрессбар
    row.rangeBar = new QProgressBar();
    row.rangeBar->setTextVisible(false);
    row.rangeBar->setFixedHeight(6);
    row.rangeBar->setRange(0, 100);
    row.rangeBar->setStyleSheet(
        "QProgressBar { background: #e0e0e0; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3498db, stop:1 #2ecc71); border-radius: 3px; }"
    );
    layout->addWidget(row.rangeBar);

    // 3. Слой подписей ПОД линией
    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->setContentsMargins(0, 2, 0, 0);

    row.minScaleLabel = new QLabel("--");
    row.maxScaleLabel = new QLabel("--");
    row.avgScaleLabel = new QLabel("--");

    QString scaleStyle = "font-size: 9px; color: #95a5a6;";
    row.minScaleLabel->setStyleSheet(scaleStyle);
    row.maxScaleLabel->setStyleSheet(scaleStyle);
    row.avgScaleLabel->setStyleSheet(scaleStyle + "font-weight: bold; color: #27ae60;");

    scaleLayout->addWidget(row.minScaleLabel); // Слева
    scaleLayout->addStretch();
    scaleLayout->addWidget(row.avgScaleLabel); // По центру
    scaleLayout->addStretch();
    scaleLayout->addWidget(row.maxScaleLabel); // Справа

    layout->addLayout(scaleLayout);

    return row;
}