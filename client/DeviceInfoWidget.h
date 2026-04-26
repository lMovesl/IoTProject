// DeviceInfoWidget.h
#ifndef DEVICEINFOWIDGET_H
#define DEVICEINFOWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include "DatabaseManager.h"

class DeviceInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceInfoWidget(QWidget* parent = nullptr);
    void setDevice(int deviceId, const QString& deviceName);
public slots:
	void displayDevice();

private:
    int m_deviceId;
    QString m_deviceName;

    QLabel* m_nameLabel;
    QLabel* m_idLabel;
    QTableWidget* m_sensorsTable;
};

#endif 