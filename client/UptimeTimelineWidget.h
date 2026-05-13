#ifndef UPTIMETIMELINEWIDGET_H
#define UPTIMETIMELINEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QDateTime>

struct DeviceStateInterval {
    qint64 startTimestamp;
    qint64 endTimestamp;   
    bool isOnline;        
};

class UptimeTimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit UptimeTimelineWidget(QWidget* parent = nullptr);

    void setData(const QVector<DeviceStateInterval>& data, qint64 rangeStart, qint64 rangeEnd);
    void setDeviceName(const QString& name);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<DeviceStateInterval> m_data;
    qint64 m_rangeStart = 0;
    qint64 m_rangeEnd = 0;

    QString m_deviceName;
};

#endif // UPTIMETIMELINEWIDGET_H