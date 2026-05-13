#ifndef FLOOR_PLAN_WIDGET_H
#define FLOOR_PLAN_WIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QMap>
#include "DeviceItem.h"

class FloorPlanWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit FloorPlanWidget(QWidget* parent = nullptr);

    void loadFloorPlan(const QString& path);
    void addDevice(int id, const QString& name, QPointF pos);
    void setDeviceAlert(int id, bool hasAlert);
    bool hasDevice(int id) const { return m_deviceItems.contains(id); }
    void centerOnDevice(int id);
    void setDeviceDisabled(int id);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void updateDevicePositionInDb(int id, QPointF pos);

signals:
    void deviceSelected(int id, const QString& name);
private:
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_background;
    QMap<int, DeviceItem*> m_deviceItems;
};

#endif