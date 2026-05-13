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
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSortingEnabled(true); 
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
    m_sourceModel->removeRows(0, m_sourceModel->rowCount());

    auto history = DatabaseManager::instance().getAllMeasurementsHistory(500);

    for (const auto& entry : history) {
        QList<QStandardItem*> rowItems;

        rowItems.append(new QStandardItem(entry.deviceName));
        rowItems.append(new QStandardItem(entry.sensorName));

        QStandardItem* valueItem = new QStandardItem();
        valueItem->setData(entry.value, Qt::UserRole);
        valueItem->setData(QString("%1 %2").arg(entry.value, 0, 'f', 2).arg(entry.unit), Qt::DisplayRole);
        rowItems.append(valueItem);

        QDateTime localTime = entry.timestamp;
        localTime.setTimeZone(QTimeZone(QTimeZone::LocalTime));

        QStandardItem* timeItem = new QStandardItem();
        timeItem->setData(localTime, Qt::UserRole);
        timeItem->setData(localTime.toString("yyyy-MM-dd HH:mm:ss"), Qt::DisplayRole);
        rowItems.append(timeItem);

        m_sourceModel->appendRow(rowItems);
    }

    m_proxyModel->sort(m_tableView->horizontalHeader()->sortIndicatorSection(),
        m_tableView->horizontalHeader()->sortIndicatorOrder());
}

void MeasurementHistoryWindow::onFilterOpened(int column, QPointF pos) {
    FilterType type = FilterType::List;
    if (column == 2) type = FilterType::NumericRange; 
    if (column == 3) type = FilterType::DateRange;    

    QStringList uniqueValues;
    if (type == FilterType::List) {
        QSet<QString> set;
        for (int i = 0; i < m_sourceModel->rowCount(); ++i)
            set.insert(m_sourceModel->index(i, column).data().toString());
        uniqueValues = set.values();
        uniqueValues.sort();
    }

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
        out.setGenerateByteOrderMark(true); 

        QStringList headers;
        for (int i = 0; i < m_proxyModel->columnCount(); ++i) {
            headers << m_sourceModel->headerData(i, Qt::Horizontal).toString();
        }
        out << headers.join(";") << "\n";

        for (int row = 0; row < m_proxyModel->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < m_proxyModel->columnCount(); ++col) {
                QModelIndex proxyIndex = m_proxyModel->index(row, col);
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