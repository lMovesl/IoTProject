#include "FloorPlanWidget.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPropertyAnimation>

FloorPlanWidget::FloorPlanWidget(QWidget* parent) : QGraphicsView(parent), m_background(nullptr) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setAcceptDrops(true);
}

void FloorPlanWidget::loadFloorPlan(const QString& path) {
    QPixmap pix(path);
    if (pix.isNull()) return;
    m_scene->clear();
    m_deviceItems.clear();
    m_background = m_scene->addPixmap(pix);
    m_scene->setSceneRect(pix.rect());
}

void FloorPlanWidget::centerOnDevice(int id) {
    if (m_deviceItems.contains(id)) {
        DeviceItem* item = m_deviceItems[id];
        centerOn(item);
        item->setSelected(true);

        // Создаем временную анимацию "вспышки"
        QPropertyAnimation* flash = new QPropertyAnimation(item, "opacity");
        flash->setDuration(200);
        flash->setStartValue(1.0);
        flash->setEndValue(0.0);
        flash->setLoopCount(4); // Мигнет 2 раза туда-обратно
        flash->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void FloorPlanWidget::addDevice(int id, const QString& name, QPointF pos) {
    if (m_deviceItems.contains(id)) return;

    DeviceItem* item = new DeviceItem(id, name, pos);
    connect(item, &DeviceItem::positionChanged, this, &FloorPlanWidget::updateDevicePositionInDb);
    connect(item, &DeviceItem::showInfoRequested, this, &FloorPlanWidget::deviceSelected);

    m_scene->addItem(item);
    m_deviceItems.insert(id, item);
}

void FloorPlanWidget::updateDevicePositionInDb(int id, QPointF pos) {
    QSqlQuery query;
    // Убедитесь, что колонки называются именно так (pos_x, pos_y)
    query.prepare("UPDATE devices SET pos_x = :x, pos_y = :y WHERE id = :id");
    query.bindValue(":x", pos.x());
    query.bindValue(":y", pos.y());
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Ошибка сохранения координат в БД:" << query.lastError().text();
    }
}

void FloorPlanWidget::setDeviceAlert(int id, bool hasAlert) {
    if (m_deviceItems.contains(id)) {
        m_deviceItems[id]->setAlert(hasAlert);
    }
}

void FloorPlanWidget::dragEnterEvent(QDragEnterEvent* event) {
    // Проверяем, что перетаскивается элемент с данными (моделью)
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    }
}

void FloorPlanWidget::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void FloorPlanWidget::dropEvent(QDropEvent* event) {
    // ИСПРАВЛЕНИЕ: используем position() вместо pos()
    QPointF dropPos = event->position();

    const QMimeData* mime = event->mimeData();
    if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encodedData = mime->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        while (!stream.atEnd()) {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

            if (roleDataMap.contains(Qt::UserRole)) {
                int deviceId = roleDataMap[Qt::UserRole].toInt();
                QString deviceName = roleDataMap[Qt::DisplayRole].toString();

                // Переводим координаты виджета в координаты сцены
                QPointF scenePos = mapToScene(dropPos.toPoint());

                addDevice(deviceId, deviceName, scenePos);
                updateDevicePositionInDb(deviceId, scenePos);
            }
        }
        event->acceptProposedAction();
    }
}