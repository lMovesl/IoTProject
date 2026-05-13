#include "DeviceItem.h"
#include <QGraphicsSceneHoverEvent>
#include <QToolTip>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>

DeviceItem::DeviceItem(int id, const QString& name, QPointF pos, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-12, -12, 24, 24, parent), m_id(id), m_name(name) {

    setPos(pos);
    setBrush(QColor(46, 204, 113));
   // setPen(QPen(Qt::white, 2));

    setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsSelectable);
    setAcceptHoverEvents(true);

    m_blinkAnim = new QPropertyAnimation(this, "opacity", this);
    m_blinkAnim->setDuration(700);
    m_blinkAnim->setStartValue(1.0);
    m_blinkAnim->setEndValue(0.2);
    m_blinkAnim->setLoopCount(-1);
    m_blinkAnim->setEasingCurve(QEasingCurve::InOutSine);
}

QVariant DeviceItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged && scene()) {
        emit positionChanged(m_id, value.toPointF());
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void DeviceItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    setToolTip(m_name);
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void DeviceItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    menu.addAction("Показать информацию", [this]() {
        emit showInfoRequested(m_id, m_name);
        });

    // Новое действие
    menu.addAction("Убрать с планировки", [this]() {
        emit removeFromMapRequested(m_id);
        });

    menu.exec(event->screenPos());
}

void DeviceItem::setDisabledState() {
    m_blinkAnim->stop(); // Останавливаем мигание
    setOpacity(0.5);     // Делаем полупрозрачным
    setBrush(QColor(127, 140, 141)); // Серый цвет (Disabled)
    update();
}

void DeviceItem::setAlert(bool active) {
    setOpacity(1.0);
    if (active) {
        setBrush(QColor(231, 76, 60)); // Ярко-красный
        if (m_blinkAnim->state() != QAbstractAnimation::Running)
            m_blinkAnim->start();
    }
    else {
        setBrush(QColor(46, 204, 113)); // Зеленый
        m_blinkAnim->stop();
    }
    update();
}