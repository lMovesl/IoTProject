#ifndef UPTIMETIMELINEWIDGET_H
#define UPTIMETIMELINEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QDateTime>

// Структура для описания одного интервала состояния
struct DeviceStateInterval {
    qint64 startTimestamp; // Unix time в секундах
    qint64 endTimestamp;   // Unix time в секундах
    bool isOnline;         // true - зеленый, false - красный
};

class UptimeTimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit UptimeTimelineWidget(QWidget* parent = nullptr);

    // Установка данных для отрисовки
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