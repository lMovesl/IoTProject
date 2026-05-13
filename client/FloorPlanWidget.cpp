#include "FloorPlanWidget.h"
#include "FloorPlanWidget.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPropertyAnimation>
#include <general/DatabaseManager.h>

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

void FloorPlanWidget::setDeviceDisabled(int id) {
    if (m_deviceItems.contains(id)) {
        m_deviceItems[id]->setDisabledState();
    }
}

void FloorPlanWidget::addDevice(int id, const QString& name, QPointF pos) {
    DeviceItem* item = new DeviceItem(id, name, pos);
    m_scene->addItem(item);
    m_deviceItems[id] = item;
    
    connect(item, &DeviceItem::positionChanged, this, [](int devId, QPointF newPos) {
        DatabaseManager::instance().updateDevicePosition(devId, newPos.x(), newPos.y());
    });
    // Соединяем сигнал удаления
    connect(item, &DeviceItem::removeFromMapRequested, this, [this](int devId) {
        DatabaseManager::instance().updateDevicePosition(devId, 0, 0);

        if (m_deviceItems.contains(devId)) {
            m_scene->removeItem(m_deviceItems[devId]);
            delete m_deviceItems.take(devId);
        }
        });

    connect(item, &DeviceItem::showInfoRequested, this, [this](int devId, QString devName) {
        emit deviceSelected(devId, devName); 
        });
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

                if (m_deviceItems.contains(deviceId)) {
                    DeviceItem* oldItem = m_deviceItems.take(deviceId);
                    m_scene->removeItem(oldItem);
                    delete oldItem;
                    qDebug() << "Дубликат удален. Перемещение устройства:" << deviceName;
                }

                QPointF scenePos = mapToScene(dropPos.toPoint());

                addDevice(deviceId, deviceName, scenePos);
                DatabaseManager::instance().updateDevicePosition(deviceId, scenePos.x(), scenePos.y());
            }
        }
        event->acceptProposedAction();
    }
}

void FloorPlanWidget::onDeviceStatusChanged(int id, bool isOnline) {
    if (m_deviceItems.contains(id)) {
        DeviceItem* item = m_deviceItems[id];
        if (isOnline) {
            item->setOpacity(1.0);
            item->setBrush(QColor(46, 204, 113));
        }
        else {
            item->setDisabledState();
        }
    }
}