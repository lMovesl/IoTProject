#ifndef DEVICE_ITEM_H
#define DEVICE_ITEM_H

#include <QGraphicsEllipseItem>
#include <QObject>
#include <QBrush>
#include <QPen>
#include <QPropertyAnimation>

class DeviceItem : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    DeviceItem(int id, const QString& name, QPointF pos, QGraphicsItem* parent = nullptr);

    int id() const { return m_id; }
    void setAlert(bool active);

signals:
    void positionChanged(int id, QPointF newPos);
    void showInfoRequested(int id, const QString& name);
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QPropertyAnimation* m_blinkAnim;
    int m_id;
    QString m_name;
};

#endif