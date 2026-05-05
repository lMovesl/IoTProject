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

    // 3. Сборка интерфейса вручную
    setupLayout();
    QMenuBar* menubar = new QMenuBar(this);
    QMenu* menu = new QMenu("Мониторинг", menubar);
    QAction* alertsHistoryAction = new QAction("История событий", menu);
    connect(alertsHistoryAction, &QAction::triggered, this, [this]() {
        AlertHistoryWindow historyWin(this);
        historyWin.exec();
        });

    setMenuBar(menubar);
    menubar->addMenu(menu);
    menu->addAction(alertsHistoryAction);

    m_model->refreshStructure();
    subscribeToAllDevices();
    m_treeView->expandToDepth(0);
}

MainWindow::~MainWindow()
{
    m_refreshTimer->stop();
    if (m_mqttClient) {
        m_mqttClient->disconnectFromHost();
        m_mqttClient->deleteLater();
    }
    // Все дочерние объекты Qt удалятся автоматически
}

void MainWindow::setupLayout()
{
    m_alertLog = new QListWidget(this);
    m_alertLog->setStyleSheet("QListWidget { background-color: #2b2b2b; color: #ffffff; font-family: Consolas; }");

    // Central widget shows device information
    m_deviceInfoWidget = new DeviceInfoWidget(this);
    setCentralWidget(m_deviceInfoWidget);

    // Tree view will be placed in a dock widget
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->header()->setSectionResizeMode(QHeaderView::Stretch);

    // Container widget for the dock
    QWidget* dockContainer = new QWidget(this);
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContainer);
    dockLayout->addWidget(m_treeView);
    dockContainer->setLayout(dockLayout);

    QDockWidget* logDock = new QDockWidget("Журнал событий", this);
    logDock->setWidget(m_alertLog);
    logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);

    m_dock = new QDockWidget(tr("Устройства"), this);
    m_dock->setWidget(dockContainer);
    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    // Connections
    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onTreeItemClicked);
    connect(m_treeView, &QTreeView::doubleClicked, this, &MainWindow::onTreeItemDoubleClicked);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this,
            &MainWindow::showContextMenu);

    // 4. Настройка таймера (обновление из БД как запасной вариант)
    connect(m_refreshTimer, &QTimer::timeout, m_deviceInfoWidget, &DeviceInfoWidget::updateData);
    connect(m_refreshTimer, &QTimer::timeout, m_model, &DeviceTreeModel::syncDevicesFromDb);
    connect(m_refreshTimer, &QTimer::timeout, m_model, &DeviceTreeModel::updateDeviceStatuses);
    m_refreshTimer->start(2000);

    setWindowTitle("IOT");
    resize(800, 600);
}

void MainWindow::onTreeItemClicked(const QModelIndex& index) {
    auto* item = m_model->itemFromIndex(index);
    if (!item || !item->parent()) return; // Пропускаем корень и комнаты

    QStandardItem* deviceItem = nullptr;
    int deviceId = -1;

    // Проверяем: нажат датчик (у него есть дедушка) или устройство (у него есть только отец-комната)
    if (item->parent()->parent() == nullptr)
        deviceItem = item;
    else
        deviceItem = item->parent();

	deviceId = deviceItem->data(Qt::UserRole).toInt();

    if (deviceId != -1 && deviceItem != nullptr) {
        // Отображаем устройство в центральном виджете
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

    bool isDevice = index.data(Qt::UserRole).isValid()
                    && !index.parent().parent().isValid();
    bool isSensor = index.parent().isValid()
                    && index.parent().parent().isValid();

    if (isRoom)
        return;
    else if (isDevice) {
        int deviceId = index.data(Qt::UserRole).toInt();
        QAction* configAction = menu.addAction("Настроить устройство");
        connect(configAction, &QAction::triggered, [this, deviceId]() {
            ConfigureDeviceDialog dialog(deviceId, this);
            if (dialog.exec() == QDialog::Accepted) {
                m_model->refreshStructure();
                subscribeToAllDevices();
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
            // Сохраняем ссылку для живых обновлений
            m_openGraphs[sensorId] = graph;
            // При удалении окна убираем ссылку
            connect(graph, &QObject::destroyed, this, [this, sensorId]() {
                m_openGraphs.remove(sensorId);
            });
            graph->show();
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::onDeviceDoubleClicked(const QModelIndex& index)
{
    QModelIndex deviceIndex = index;
    if (index.parent().isValid())
        deviceIndex = index.parent();

    int deviceId = deviceIndex.data(Qt::UserRole).toInt();
    ConfigureDeviceDialog dialog(deviceId, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_model->refreshStructure();
        subscribeToAllDevices();
    }
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
    // Отписываемся от всех старых топиков (просто очистим карту и создадим новые подписки)
    QMapIterator<int, QString> it(m_deviceTopics);
    while (it.hasNext()) {
        it.next();
        m_mqttClient->unsubscribe(it.value());
    }
    m_deviceTopics.clear();

    if (!DatabaseManager::instance().open()) return;
    QList<DeviceInfo> devices = DatabaseManager::instance().getDevicesByRoom(0); // комната 0 - нераспределенные
    // Также нужно получить устройства из всех комнат
    QList<RoomInfo> rooms = DatabaseManager::instance().getRooms();
    for (const RoomInfo& room : rooms) {
        QList<DeviceInfo> devs = DatabaseManager::instance().getDevicesByRoom(room.id);
        devices.append(devs);
    }
    // Убираем дубликаты (по id)
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
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON payload:" << payload;
        return;
    }
    QJsonObject obj = doc.object();

    // Expect format: devices/<deviceId>/data
    QStringList parts = topic.split('/');
    if (parts.size() < 3) {
        qDebug() << "Unexpected topic format:" << topic;
        return;
    }
    bool ok;
    int deviceIdFromTopic = parts[1].toInt(&ok);
    if (!ok) {
        qDebug() << "Cannot parse deviceId from topic:" << topic;
        return;
    }

    // Use full topic string as unique identifier
    int deviceId = DatabaseManager::instance().getOrCreateDevice(topic);
    // Ensure subscription exists
    subscribeToDevice(deviceId);

    QDateTime ts = QDateTime::currentDateTime(); // arrival time

    // Iterate over key/value pairs
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
        } else {
            value = val.toDouble();
            unit = "?";
        }

        int sensorId = DatabaseManager::instance().getOrCreateSensor(deviceId, sensorKey, unit);
        if (m_openGraphs.contains(sensorId)) {
            m_openGraphs[sensorId]->appendPoint(value, ts);
        }
    }
}

void MainWindow::openGraphWindow(int sensorId, const QString& sensorName)
{
    // Этот метод сейчас не используется напрямую, оставляем для расширения
    SensorGraphWindow* graph = new SensorGraphWindow(sensorId, sensorName);
    graph->setAttribute(Qt::WA_DeleteOnClose);
    m_openGraphs[sensorId] = graph;
    connect(graph, &QObject::destroyed, this,
            [this, sensorId]() { m_openGraphs.remove(sensorId); });
    graph->show();
}

void MainWindow::handleAlertMessage(const QByteArray& message, const QMqttTopicName& topic) {
    // Проверяем, что это действительно топик алерта
    QStringList parts = topic.name().split('/');
    if (parts.size() == 3 && parts.last() == "alerts") {
        QString deviceMac = parts.at(1); // Достаем MAC-адрес из середины топика

        QJsonDocument doc = QJsonDocument::fromJson(message);
        QJsonObject obj = doc.object();

        QString devName = obj["device_name"].toString();
        QString sensor = obj["sensor"].toString();
        QString type = obj["type"].toString();
        double val = obj["value"].toDouble();

        // Формируем запись для твоего m_alertLog (созданного кодом)
        QString logMsg = QString("[%1] MAC: %2 (%3) -> %4: %5 (Знач: %6)")
            .arg(QTime::currentTime().toString("HH:mm:ss"))
            .arg(deviceMac)
            .arg(devName)
            .arg(sensor)
            .arg(type)
            .arg(val);

        QListWidgetItem* item = new QListWidgetItem(logMsg);
        item->setForeground(QBrush(QColor("#ff5c5c")));
        m_alertLog->insertItem(0, item);
    }
}

void MainWindow::onTreeItemDoubleClicked(const QModelIndex& index) {
    // Проверяем, что кликнули именно по датчику, а не по комнате или устройству
    // В вашей модели это обычно определяется через custom roles или тип данных
    int sensorId = index.data(Qt::UserRole + 1).toInt(); // Предполагаем, что ID хранится в Role

    if (sensorId > 0) {
        SensorConfigDialog dialog(sensorId, this);
        if (dialog.exec() == QDialog::Accepted) {
            // Можно обновить статус-бар или дерево, если настройки изменились
            m_alertLog->insertItem(0, "Настройки датчика обновлены");
        }
    }
}