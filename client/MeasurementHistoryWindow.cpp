#include "MeasurementHistoryWindow.h"
#include "DatabaseManager.h"
#include "FilterHeader.h"
#include "FilterPopup.h"

#include <QLineEdit>
#include <QMenu>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QMenuBar>

MeasurementHistoryWindow::MeasurementHistoryWindow(QWidget* parent) : QDialog(parent) {
    resize(1000, 600);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_sourceModel = new QStandardItemModel(0, 4, this);
    m_sourceModel->setHorizontalHeaderLabels({ "Устройство", "Датчик", "Значение", "Время" });

    m_proxyModel = new MultiColumnFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_sourceModel);
    
    m_tableView = new QTableView(this);
    FilterHeader* header = new FilterHeader(Qt::Horizontal, m_tableView);
    m_tableView->setHorizontalHeader(header);
    m_tableView->setModel(m_proxyModel); // Ставим прокси-модель!
    m_tableView->setSortingEnabled(true); // Сортировка работает "из коробки"
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);
    
    QMenuBar* menuBar = new QMenuBar(this);
    QMenu* menu = new QMenu("&Файл", menuBar);
    QAction* actionExport = new QAction("Экспорт в CSV", this);
    menuBar->addMenu(menu);
    menu->addAction(actionExport);

    mainLayout->setMenuBar(menuBar);
    mainLayout->addWidget(m_tableView, 1);

    loadHistory();

    connect(actionExport, &QAction::triggered, this, &MeasurementHistoryWindow::exportToCsv);
    connect(header, &FilterHeader::filterClicked, this, &MeasurementHistoryWindow::onFilterOpened);
}

void MeasurementHistoryWindow::loadHistory() {
    // 1. Очищаем старые данные из исходной модели
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());

    // 2. Получаем данные из базы (например, последние 500 записей)
    auto history = DatabaseManager::instance().getAllMeasurementsHistory(500);

    for (const auto& entry : history) {
        // Создаем список элементов для одной строки
        QList<QStandardItem*> rowItems;

        // Колонка 0: Устройство (Текст)
        rowItems.append(new QStandardItem(entry.deviceName));

        // Колонка 1: Датчик (Текст)
        rowItems.append(new QStandardItem(entry.sensorName));

        // Колонка 2: Значение (Число + Единица измерения)
        QStandardItem* valueItem = new QStandardItem();
        valueItem->setData(entry.value, Qt::UserRole);
        valueItem->setData(QString("%1 %2").arg(entry.value, 0, 'f', 2).arg(entry.unit), Qt::DisplayRole);
        rowItems.append(valueItem);

        QDateTime localTime = entry.timestamp;
        localTime.setTimeZone(QTimeZone(QTimeZone::LocalTime));

        QStandardItem* timeItem = new QStandardItem();
        timeItem->setData(localTime, Qt::UserRole); // Для фильтра и сортировки как объект даты
        timeItem->setData(localTime.toString("yyyy-MM-dd HH:mm:ss"), Qt::DisplayRole);
        rowItems.append(timeItem);

        // Добавляем строку в ИСХОДНУЮ модель
        m_sourceModel->appendRow(rowItems);
    }

    // После загрузки данных полезно принудительно попросить прокси-модель пересортировать данные, 
    // если сортировка уже была включена пользователем.
    m_proxyModel->sort(m_tableView->horizontalHeader()->sortIndicatorSection(),
        m_tableView->horizontalHeader()->sortIndicatorOrder());
}

void MeasurementHistoryWindow::onFilterOpened(int column, QPointF pos) {
    FilterType type = FilterType::List;
    if (column == 2) type = FilterType::NumericRange; // Колонка "Значение"
    if (column == 3) type = FilterType::DateRange;    // Колонка "Время"

    QStringList uniqueValues;
    if (type == FilterType::List) {
        QSet<QString> set;
        for (int i = 0; i < m_sourceModel->rowCount(); ++i)
            set.insert(m_sourceModel->index(i, column).data().toString());
        uniqueValues = set.values();
        uniqueValues.sort();
    }

    // Вызываем диалог, передавая текущее состояние фильтра
    FilterPopup* popup = new FilterPopup(type, m_currentFilters[column], uniqueValues, this);
    popup->move(pos.toPoint());

    if (popup->exec() == QDialog::Accepted) {
        ColumnFilter newFilter = popup->getFilterData();

        m_currentFilters[column] = newFilter;
        m_proxyModel->setFilter(column, newFilter);
    }
}

void MeasurementHistoryWindow::exportToCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт измерений",
        "measurements_history.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setGenerateByteOrderMark(true); // Для корректного открытия в Excel

        // 1. Экспорт заголовков
        QStringList headers;
        for (int i = 0; i < m_proxyModel->columnCount(); ++i) {
            headers << m_sourceModel->headerData(i, Qt::Horizontal).toString();
        }
        out << headers.join(";") << "\n";

        // 2. Экспорт данных с учетом фильтрации и сортировки
        // Используем rowCount именно прокси-модели
        for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < m_proxyModel->columnCount(); ++col) {
                // Получаем индекс прокси-модели
                QModelIndex proxyIndex = m_proxyModel->index(row, col);
                // Берем отображаемое значение
                rowData << proxyIndex.data(Qt::DisplayRole).toString();
            }
            out << rowData.join(";") << "\n";
        }

        file.close();
        QMessageBox::information(this, "Готово", "Данные успешно экспортированы.");
    }
    else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл.");
    }
}