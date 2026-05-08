#ifndef FILTER_HEADER_H
#define FILTER_HEADER_H

#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

class FilterHeader : public QHeaderView {
    Q_OBJECT
public:
    FilterHeader(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QHeaderView(orientation, parent) {
        setSectionsClickable(true);
    }

signals:
    void filterClicked(int section, QPointF globalPos);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int sectionId) const override {
        painter->save();
        QHeaderView::paintSection(painter, rect, sectionId);
        painter->restore();

        // Рисуем иконку фильтра (воронку) в правой части секции
        QRect filterRect = getFilterButtonRect(rect);
        painter->drawPixmap(filterRect, QPixmap(":/icons/filter.svg")); // Нужно добавить иконку в ресурсы
    }

    void mousePressEvent(QMouseEvent* event) override {
        int section = logicalIndexAt(event->pos());
        // sectionRect возвращает координаты внутри заголовка
        QRect rect = QRect(sectionViewportPosition(section), 0, sectionSize(section), height());

        if (getFilterButtonRect(rect).contains(event->pos())) {
            emit filterClicked(section, event->globalPosition());
        }
        else {
            QHeaderView::mousePressEvent(event);
        }
    }

private:
    QRect getFilterButtonRect(const QRect& sectionRect) const {
        return QRect(sectionRect.right() - 20, sectionRect.center().y() - 8, 16, 16);
    }
};

#endif //!FILTER_HEADER_H