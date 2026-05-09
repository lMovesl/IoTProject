#include "UptimeTimelineWidget.h"
#include <QPainter>
#include <QPaintEvent>

UptimeTimelineWidget::UptimeTimelineWidget(QWidget* parent) : QWidget(parent) {
    // Минимальная высота виджета, чтобы он не схлопнулся
    setMinimumHeight(80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void UptimeTimelineWidget::setData(const QVector<DeviceStateInterval>& data, qint64 rangeStart, qint64 rangeEnd) {
    m_data = data;
    m_rangeStart = rangeStart;
    m_rangeEnd = rangeEnd;
    update(); // Команда на перерисовку виджета
}

void UptimeTimelineWidget::setDeviceName(const QString& name) {
    m_deviceName = name;
    update();
}

void UptimeTimelineWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Настройки отступов
    int leftMargin = 120;  // Место под текст слева
    int rightMargin = 20;  // Отступ справа
    int topMargin = 15;    // Отступ сверху
    int bottomMargin = 25; // Место под время снизу

    int barWidth = w - leftMargin - rightMargin;
    int barHeight = h - topMargin - bottomMargin;

    // 1. Отрисовка рамки самого виджета (как на изображении)
    painter.setPen(QColor(220, 220, 220));
    painter.drawRect(0, 0, w - 1, h - 1);

    // 2. Отрисовка названия устройства
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    // Рисуем две строки текста слева
    QRect textRect(10, topMargin, leftMargin - 20, barHeight / 2);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignBottom, m_deviceName);

    // Если нет данных или диапазона, выходим
    if (m_rangeStart >= m_rangeEnd || barWidth <= 0) return;

    qint64 totalDuration = m_rangeEnd - m_rangeStart;

    // 3. Фон шкалы (серый для участков, где нет данных)
    QRect barRect(leftMargin, topMargin, barWidth, barHeight);
    painter.fillRect(barRect, QColor(180, 180, 180));

    // 4. Отрисовка зеленых и красных интервалов
    for (const auto& status : m_data) {
        // Обрезаем интервал по краям отображаемого диапазона
        qint64 start = qMax(status.startTimestamp, m_rangeStart);
        qint64 end = qMin(status.endTimestamp, m_rangeEnd);

        if (start >= end) continue; // Интервал вне видимой зоны

        // Вычисляем координаты по оси X в процентах
        double startRatio = static_cast<double>(start - m_rangeStart) / totalDuration;
        double endRatio = static_cast<double>(end - m_rangeStart) / totalDuration;

        int xStart = leftMargin + (startRatio * barWidth);
        int xEnd = leftMargin + (endRatio * barWidth);

        QRect segmentRect(xStart, topMargin, xEnd - xStart, barHeight);

        QColor color = status.isOnline ? QColor(136, 190, 76) : QColor(217, 83, 79);
        painter.fillRect(segmentRect, color);
    }

    painter.setPen(QColor(150, 150, 150));
    int numTicks = 10; // Количество отметок времени
    for (int i = 0; i <= numTicks; ++i) {
        int x = leftMargin + (i * barWidth / numTicks);

        // Рисуем маленькую черточку
        painter.drawLine(x, topMargin + barHeight, x, topMargin + barHeight + 4);

        // Вычисляем время для этой отметки
        qint64 tickTime = m_rangeStart + (i * totalDuration / numTicks);
        QString timeStr = QDateTime::fromSecsSinceEpoch(tickTime).toString("HH:mm");

        // Выводим текст по центру отметки
        QRect timeRect(x - 20, topMargin + barHeight + 5, 40, 20);
        painter.setPen(QColor(80, 80, 80));
        painter.drawText(timeRect, Qt::AlignCenter, timeStr);
        painter.setPen(QColor(150, 150, 150));
    }
}