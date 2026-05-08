#ifndef FILTER_POPUP_H
#define FILTER_POPUP_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QStackedWidget>

#include "MultiColumnFilterProxyModel.h"

class FilterPopup : public QDialog {
    Q_OBJECT
public:
    explicit FilterPopup(FilterType type, const ColumnFilter& current, const QStringList& uniqueValues, QWidget* parent)
        : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint), m_type(type) {

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(6);

        // Это заставит диалог ВСЕГДА принимать минимально возможный размер под добавленные элементы
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);

        if (m_type == FilterType::List) {
            // --- Создаем только Список ---
            m_listWidget = new QListWidget(this);
            for (const QString& val : uniqueValues) {
                auto* item = new QListWidgetItem(val, m_listWidget);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(current.allowedValues.isEmpty() || current.allowedValues.contains(val) ? Qt::Checked : Qt::Unchecked);
            }

            m_listWidget->setFixedHeight(200); // Жестко ограничиваем высоту скролла
            m_listWidget->setFixedWidth(200);  // И ширину списка
            mainLayout->addWidget(m_listWidget);

        }
        else if (m_type == FilterType::NumericRange) {
            // --- Создаем только Числовой фильтр ---
            m_minSpin = new QDoubleSpinBox(this);
            m_maxSpin = new QDoubleSpinBox(this);
            m_minSpin->setRange(-999999.0, 999999.0);
            m_maxSpin->setRange(-999999.0, 999999.0);
            m_minSpin->setValue(current.isActive ? current.minNum : -999999.0);
            m_maxSpin->setValue(current.isActive ? current.maxNum : 999999.0);

            mainLayout->addWidget(new QLabel("Минимум:", this));
            mainLayout->addWidget(m_minSpin);
            mainLayout->addWidget(new QLabel("Максимум:", this));
            mainLayout->addWidget(m_maxSpin);

        }
        else if (m_type == FilterType::DateRange) {
            // --- Создаем только Календари ---
            m_minDate = new QDateTimeEdit(this);
            m_maxDate = new QDateTimeEdit(this);
            m_minDate->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
            m_maxDate->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
            m_minDate->setCalendarPopup(true);
            m_maxDate->setCalendarPopup(true);

            m_minDate->setDateTime(current.isActive && current.minDate.isValid() ? current.minDate : QDateTime::currentDateTime().date().startOfDay());
            m_maxDate->setDateTime(current.isActive && current.maxDate.isValid() ? current.maxDate : QDateTime::currentDateTime().date().endOfDay());

            mainLayout->addWidget(new QLabel("С:", this));
            mainLayout->addWidget(m_minDate);
            mainLayout->addWidget(new QLabel("По:", this));
            mainLayout->addWidget(m_maxDate);
        }

        // Кнопки добавляем всегда
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setStyleSheet("QDialog { border: 1px solid gray; background: white; }");
    }

    ColumnFilter getFilterData() {
        ColumnFilter f;
        f.type = m_type;
        f.isActive = true;
        if (m_type == FilterType::List) {
            for (int i = 0; i < m_listWidget->count(); ++i)
                if (m_listWidget->item(i)->checkState() == Qt::Checked) f.allowedValues.insert(m_listWidget->item(i)->text());
        }
        else if (m_type == FilterType::NumericRange) {
            f.minNum = m_minSpin->value(); 
            f.maxNum = m_maxSpin->value();
        }
        else {
            f.minDate = m_minDate->dateTime(); 
            f.maxDate = m_maxDate->dateTime();
        }
        return f;
    }

    QSet<QString> getSelectedValues() const {
        QSet<QString> selected;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->checkState() == Qt::Checked)
                selected.insert(m_listWidget->item(i)->text());
        }
        return selected;
    }

private:
    FilterType m_type;
    QListWidget* m_listWidget = nullptr;
    QDoubleSpinBox* m_minSpin = nullptr;
    QDoubleSpinBox* m_maxSpin = nullptr;
    QDateTimeEdit* m_minDate = nullptr;
    QDateTimeEdit* m_maxDate = nullptr;
};

#endif //!FILTER_POPUP_H