#include "MainWindow.h"
#include <QMessageBox>
#include <QMenu>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QMenuBar>

#include "ConfigureDeviceDialog.h"
#include "SensorGraphWindow.h"
#include "SensorConfigDialog.h"
#include "AlertHistoryWindow.h"
#include "MeasurementHistoryWindow.h"

#include <QFileDialog>
#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_model(new DeviceTreeModel(this)),
      m_refreshTimer(new QTimer(this)),
      m_mqttClient(new QMqttClient(this))
{
    m_mqttClient->setHostname("localhost");
    m_mqttClient->setPort(1883);

    connect(m_mqttClient, &QMqttClient::connected, this, [this]() {
        m_mqttClient->subscribe(QMqttTopicFilter("devices/+/alerts"));
        qDebug() << "MQTT клиент подключен";
    });
    connect(m_mqttClient, &QMqttClient::disconnected, this, []() {
        qDebug() << "MQTT клиент отключен";
    });
    connect(m_mqttClient, &QMqttClient::messageReceived, this, &MainWindow::handleAlertMessage);
    connect(m_mqttClient, &QMqttClient::messageReceived, this,
            [this](const QByteArray& payload, const QMqttTopicName& topic) {
                handleMqttMessage(topic.name(), payload);
            });
    m_mqttClient->connectToHost();

    setupLayout();
    QMenuBar* menubar = new QMenuBar(this);
    QMenu* menu = new QMenu("Мониторинг", menubar);
    QAction* measurmentsHistoryAction = new QAction("История измерений", menu);
    connect(measurmentsHistoryAction, &QAction::triggered, this, [this]() {
        MeasurementHistoryWindow historyWin(this);
        historyWin.exec();
        });

    QAction* alertsHistoryAction = new QAction("История событий", menu);
    connect(alertsHistoryAction, &QAction::triggered, this, [this]() {
        AlertHistoryWindow historyWin(this);
        historyWin.exec();
        });

    QMenu* viewMenu = new QMenu("Вид", menubar);
    QAction* loadPlanAction = viewMenu->addAction("Загрузить планировку...");
    loadPlanAction->setShortcut(QKeySequence("Ctrl+O"));

    connect(loadPlanAction, &QAction::triggered, this, &MainWindow::onSelectFloorPlan);
    connect(m_deviceInfoWidget->getFloorPlan(), &FloorPlanWidget::deviceSelected,
        this, [this](int id, const QString& name) {
            m_deviceInfoWidget->setDevice(id, name);
            m_deviceInfoWidget->updateData();
        });

    setMenuBar(menubar);
    menubar->addMenu(menu);
    menubar->addMenu(viewMenu);
    menu->addAction(measurmentsHistoryAction);
    menu->addAction(alertsHistoryAction);

    m_model->refreshStructure();
    subscribeToAllDevices();
    m_treeView->expandToDepth(0);

    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(false); // Дерево только отдает
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);

    QSettings settings("MyCompany", "IoTSystem");
    QString savedPath = settings.value("floorPlanPath").toString();
    if (!savedPath.isEmpty()) {
        m_deviceInfoWidget->loadFloorPlan(savedPath);
        m_deviceInfoWidget->loadDevicesToMap();
    }
}

void MainWindow::updateAllVisualStatuses() {
    // Обновляем дерево (вызываем существующий метод модели)
    m_model->updateDeviceStatuses();

    // Обновляем карту
    QList<DeviceInfo> devices = DatabaseManager::instance().getAllDevices();
    for (const auto& dev : devices) {
        bool isOnline = DatabaseManager::instance().isDeviceOnline(dev.id);
        // Если онлайн -> alert = false (зеленый), если оффлайн -> alert = true (красный)
        m_deviceInfoWidget->getFloorPlan()->setDeviceAlert(dev.id, !isOnline);
    }
}

MainWindow::~MainWindow()
{
    m_refreshTimer->stop();
    if (m_mqttClient) {
        m_mqttClient->disconnectFromHost();
        m_mqttClient->deleteLater();
    }
}

void MainWindow::setupLayout()
{
    m_alertLog = new QListWidget(this);
    m_alertLog->setStyleSheet("QListWidget { color: #ffffff; font-family: Consolas; }");

    m_deviceInfoWidget = new DeviceInfoWidget(this);
    setCentralWidget(m_deviceInfoWidget);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->header()->setSectionResizeMode(QHeaderView::Stretch);

    QWidget* dockContainer = new QWidget(this);
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContainer);
    dockLayout->addWidget(m_treeView);
    dockContainer->setLayout(dockLayout);

    QDockWidget* logDock = new QDockWidget("Журнал событий", this);
    logDock->setWidget(m_alertLog);
    logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    m_dock = new QDockWidget(tr("Устройства"), this);
    m_dock->setWidget(dockContainer);
    m_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onTreeItemClicked);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this,
            &MainWindow::showContextMenu);

    connect(m_refreshTimer, &QTimer::timeout, m_deviceInfoWidget, &DeviceInfoWidget::updateData);
    connect(m_refreshTimer, &QTimer::timeout, m_model, &DeviceTreeModel::syncDevicesFromDb);
    connect(m_refreshTimer, &QTimer::timeout, m_model, &DeviceTreeModel::updateDeviceStatuses);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::updateAllVisualStatuses);
    m_refreshTimer->start(5000);

    setWindowTitle("IOT");
    resize(800, 600);
}

void MainWindow::onTreeItemClicked(const QModelIndex& index) {
    auto* item = m_model->itemFromIndex(index);
    if (!item || !item->parent()) return;

    QStandardItem* deviceItem = nullptr;
    int deviceId = -1;

    if (item->parent()->parent() == nullptr)
        deviceItem = item;
    else
        deviceItem = item->parent();

	deviceId = deviceItem->data(Qt::UserRole).toInt();

    if (deviceId != -1 && deviceItem != nullptr && deviceId != m_deviceInfoWidget->getCurrentIDDevice()) {
        m_deviceInfoWidget->setDevice(deviceId, deviceItem->text());
        m_deviceInfoWidget->updateData();
    }
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);

    bool isRoom = !index.parent().isValid();
    // Устройство — это второй уровень вложенности
    bool isDevice = index.data(Qt::UserRole).isValid()
        && index.parent().isValid()
        && !index.parent().parent().isValid();
    // Датчик — это третий уровень вложенности
    bool isSensor = index.parent().isValid()
        && index.parent().parent().isValid();

    if (isRoom) {
        return;
    }
    else if (isDevice) {
        int deviceId = index.data(Qt::UserRole).toInt();

        // --- НОВАЯ ЛОГИКА: Поиск на карте ---
        QAction* findAction = menu.addAction("Показать на планировке");

        // Проверяем, добавлено ли устройство на карту
        bool onMap = m_deviceInfoWidget->getFloorPlan()->hasDevice(deviceId);
        findAction->setEnabled(onMap); // Если нет на сцене - кнопка неактивна

        if (onMap) {
            connect(findAction, &QAction::triggered, [this, deviceId]() {
                m_deviceInfoWidget->getFloorPlan()->centerOnDevice(deviceId);
                });
        }
        menu.addSeparator();
        // ------------------------------------

        QAction* configAction = menu.addAction("Настроить устройство");
        connect(configAction, &QAction::triggered, [this, deviceId]() {
            ConfigureDeviceDialog dialog(deviceId, this);
            if (dialog.exec() == QDialog::Accepted) {
                m_model->refreshStructure();
                subscribeToAllDevices(); // убедитесь, что метод доступен
            }
            });
    }
    else if (isSensor) {
        int sensorId = index.data(Qt::UserRole + 1).toInt();
        QString sensorName = index.data(Qt::DisplayRole).toString();

        QAction* graphAction = menu.addAction("Показать график");
        connect(graphAction, &QAction::triggered, [this, sensorId, sensorName]() {
            SensorGraphWindow* graph = new SensorGraphWindow(sensorId, sensorName);
            graph->setAttribute(Qt::WA_DeleteOnClose);
            m_openGraphs[sensorId] = graph;
            connect(graph, &QObject::destroyed, this, [this, sensorId]() {
                m_openGraphs.remove(sensorId);
                });
            graph->show();
            });

        QAction* alertAction = menu.addAction("Установить пороговые значения");
        connect(alertAction, &QAction::triggered, [this, sensorId]() {
            if (sensorId > 0) {
                SensorConfigDialog dialog(sensorId, this);
                if (dialog.exec() == QDialog::Accepted) {
                    m_alertLog->insertItem(0, "Настройки датчика обновлены");
                }
            }
            });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::subscribeToDevice(int deviceId)
{
    QString topic = QStringLiteral("devices/%1/data").arg(deviceId);
    if (!m_deviceTopics.contains(deviceId)) {
        m_deviceTopics[deviceId] = topic;
        m_mqttClient->subscribe(topic);
        qDebug() << "Подписались на топик:" << topic;
    }
}

void MainWindow::subscribeToAllDevices()
{
    QMapIterator<int, QString> it(m_deviceTopics);
    while (it.hasNext()) {
        it.next();
        m_mqttClient->unsubscribe(it.value());
    }
    m_deviceTopics.clear();

    if (!DatabaseManager::instance().open()) return;
    QList<DeviceInfo> devices = DatabaseManager::instance().getDevicesByRoom(0); 
    QList<RoomInfo> rooms = DatabaseManager::instance().getRooms();
    for (const RoomInfo& room : rooms) {
        QList<DeviceInfo> devs = DatabaseManager::instance().getDevicesByRoom(room.id);
        devices.append(devs);
    }
    QSet<int> seen;
    for (const DeviceInfo& dev : devices) {
        if (!seen.contains(dev.id)) {
            seen.insert(dev.id);
            subscribeToDevice(dev.id);
        }
    }
}

void MainWindow::handleMqttMessage(const QString& topic, const QByteArray& payload)
{
    QStringList parts = topic.split('/');
    if (parts.size() < 3) {
        qDebug() << "Unexpected topic format:" << topic;
        return;
    }

    QString deviceMac = parts[1];
    QString messageType = parts[2]; 

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid JSON payload from topic:" << topic;
        return;
    }
    QJsonObject obj = doc.object();

    int deviceId = DatabaseManager::instance().getOrCreateDevice(deviceMac);

    subscribeToDevice(deviceId);

    if (messageType == "alerts") {
        return;
    }

    QDateTime ts = QDateTime::currentDateTime();

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString sensorKey = it.key();
        QJsonValue val = it.value();
        double value = 0.0;
        QString unit = "?";

        if (val.isObject()) {
            QJsonObject sensorObj = val.toObject();
            value = sensorObj["value"].toDouble();
            if (sensorObj.contains("unit"))
                unit = sensorObj["unit"].toString();
        }
        else {
            value = val.toDouble();
        }

        int sensorId = DatabaseManager::instance().getOrCreateSensor(deviceId, sensorKey, unit);

        if (m_openGraphs.contains(sensorId)) {
            auto* graph = m_openGraphs[sensorId];
            graph->appendPoint(value, ts);

            double predicted = DatabaseManager::instance().predictFutureValue(sensorId, 300, 10);
            if (!std::isnan(predicted)) {
                graph->getChart()->updatePrediction(ts, value, 300, predicted);
            }
            else {
                graph->getChart()->clearPrediction();
            }
        }
    }
}

void MainWindow::openGraphWindow(int sensorId, const QString& sensorName)
{
    SensorGraphWindow* graph = new SensorGraphWindow(sensorId, sensorName);
    graph->setAttribute(Qt::WA_DeleteOnClose);
    m_openGraphs[sensorId] = graph;
    connect(graph, &QObject::destroyed, this,
            [this, sensorId]() { m_openGraphs.remove(sensorId); });
    graph->show();
}

void MainWindow::handleAlertMessage(const QByteArray& message, const QMqttTopicName& topic) {
    QStringList parts = topic.name().split('/');
    if (parts.size() == 3 && parts.last() == "alerts") {
        QString deviceMac = parts.at(1); // Достаем MAC-адрес из середины топика

        QJsonDocument doc = QJsonDocument::fromJson(message);
        QJsonObject obj = doc.object();

        QString devName = obj["device_name"].toString();
        QString sensor = obj["sensor"].toString();
        QString type = obj["type"].toString();
        double val = obj["value"].toDouble();

        QString logMsg = QString("[%1] Имя: %2 -> %3: %4 (Знач: %5)")
            .arg(QTime::currentTime().toString("HH:mm:ss"))
            .arg(devName)
            .arg(sensor)
            .arg(type)
            .arg(val);

        QListWidgetItem* item = new QListWidgetItem(logMsg);
        item->setForeground(QBrush(QColor("#ff5c5c")));
        m_alertLog->insertItem(0, item);
    }
}



void MainWindow::onSelectFloorPlan() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Выберите изображение планировки",
        "",
        "Изображения (*.png *.jpg *.jpeg *.bmp);;Все файлы (*.*)");

    if (!fileName.isEmpty()) {
        m_deviceInfoWidget->loadFloorPlan(fileName);

        QSettings settings("MyCompany", "IoTSystem");
        settings.setValue("floorPlanPath", fileName);
    }
}