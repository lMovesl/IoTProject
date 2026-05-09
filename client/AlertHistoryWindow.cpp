#include "AlertHistoryWindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QDateTime>
#include "general/DatabaseManager.h"

AlertHistoryWindow::AlertHistoryWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("История аномалий");
    resize(1000, 600);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // 1. Инициализация моделей
    m_sourceModel = new QStandardItemModel(this);
    m_sourceModel->setHorizontalHeaderLabels({ "Устройство", "Датчик", "Значение", "Ед. изм.", "Тип", "Время" });

    m_proxyModel = new MultiColumnFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_sourceModel);

    // 2. Настройка таблицы
    m_view = new QTableView(this);
    m_view->setModel(m_proxyModel);

    // Установка кастомного заголовка для фильтров
    FilterHeader* header = new FilterHeader(Qt::Horizontal, m_view);
    m_view->setHorizontalHeader(header);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSortingEnabled(true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    loadAlerts();

    layout->addWidget(m_view);

    // Кнопки
    QPushButton* btnClose = new QPushButton("Закрыть");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(btnClose);

    // Соединяем сигнал клика по иконке фильтра
    connect(header, &FilterHeader::filterClicked, this, &AlertHistoryWindow::onFilterOpened);
}

void AlertHistoryWindow::loadAlerts() {
    auto history = DatabaseManager::instance().getAlertHistory(500); // Загрузим побольше для теста фильтров
    m_sourceModel->setRowCount(0);

    for (const auto& entry : history) {
        QList<QStandardItem*> row;

        row << new QStandardItem(entry.deviceName);
        row << new QStandardItem(entry.sensorKey);

        QStandardItem* valItem = new QStandardItem(QString::number(entry.value, 'f', 2));
        valItem->setData(entry.value, Qt::UserRole);
        row << valItem;

        row << new QStandardItem(entry.unit);
        row << new QStandardItem(entry.type);

        QDateTime localTime = entry.timestamp;
        localTime.setTimeZone(QTimeZone(QTimeZone::LocalTime));

        QStandardItem* timeItem = new QStandardItem();
        timeItem->setData(localTime, Qt::UserRole); // Для фильтра и сортировки как объект даты
        timeItem->setData(localTime.toString("yyyy-MM-dd HH:mm:ss"), Qt::DisplayRole);
        row << timeItem;

        m_sourceModel->appendRow(row);
    }
}

void AlertHistoryWindow::onFilterOpened(int column, QPointF pos) {
    FilterType type = FilterType::List;
    if (column == 5) type = FilterType::DateRange;    // Время
    if (column == 2) type = FilterType::NumericRange; // Значение

    QStringList uniqueValues;
    if (type == FilterType::List) {
        QSet<QString> set;
        for (int i = 0; i < m_sourceModel->rowCount(); ++i)
            set.insert(m_sourceModel->index(i, column).data(Qt::DisplayRole).toString());
        uniqueValues = set.values();
        uniqueValues.sort();
    }

    ColumnFilter currentFilter = m_currentFilters.value(column);
    FilterPopup popup(type, currentFilter, uniqueValues, this);
    popup.move(pos.toPoint());

    if (popup.exec() == QDialog::Accepted) {
        ColumnFilter newFilter = popup.getFilterData();
        m_currentFilters[column] = newFilter;

        m_proxyModel->setFilter(column, newFilter);
    }
}