#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QMqttClient>
#include <QDockWidget>
#include <QListWidget>

#include "DeviceInfoWidget.h"
#include "DatabaseManager.h"
#include "DeviceTreeModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void showContextMenu(const QPoint& pos);
    void handleAlertMessage(const QByteArray& message, const QMqttTopicName& topic);
    void handleMqttMessage(const QString& topic, const QByteArray& payload);
    void openGraphWindow(int sensorId, const QString& sensorName);
    void onTreeItemClicked(const QModelIndex& index);
private:
    // Элементы интерфейса
    QTreeView* m_treeView;
    QDockWidget* m_dock;

    DeviceTreeModel* m_model;
    QTimer* m_refreshTimer;
    QMqttClient* m_mqttClient;
    DeviceInfoWidget* m_deviceInfoWidget;

    QListWidget* m_alertLog;

    // Mapping deviceId -> MQTT topic
    QMap<int, QString> m_deviceTopics;
    // Mapping sensorId -> open graph window (to update live)
    QMap<int, class SensorGraphWindow*> m_openGraphs;

    void setupLayout(); // Метод для ручной сборки интерфейса
    void onDeviceDoubleClicked(const QModelIndex& index);
    void subscribeToDevice(int deviceId);
    void subscribeToAllDevices();
};

#endif