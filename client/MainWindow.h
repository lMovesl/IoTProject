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
    void onSelectFloorPlan();
    void updateAllVisualStatuses();
    void performFullUpdate();
private:
    QTreeView* m_treeView;
    QDockWidget* m_dock;

    DeviceTreeModel* m_model;
    QTimer* m_refreshTimer;
    QMqttClient* m_mqttClient;
    DeviceInfoWidget* m_deviceInfoWidget;

    QListWidget* m_alertLog;

    QMap<int, QString> m_deviceTopics;
    QMap<int, class SensorGraphWindow*> m_openGraphs;

    void setupLayout(); 
    void subscribeToDevice(int deviceId);
    void subscribeToAllDevices();
};

#endif