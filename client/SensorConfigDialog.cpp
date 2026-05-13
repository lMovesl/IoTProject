#include "SensorConfigDialog.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <general/DatabaseManager.h>

SensorConfigDialog::SensorConfigDialog(int sensorId, QWidget* parent)
    : QDialog(parent), m_sensorId(sensorId)
{
    setWindowTitle("Настройка аналитики датчика");
    setMinimumWidth(300);

    m_chkMin = new QCheckBox("Нижний порог:");
    m_chkMax = new QCheckBox("Верхний порог:");
    m_chkRate = new QCheckBox("Макс. скачок/сек:");

    m_spinMin = new QDoubleSpinBox();
    m_spinMax = new QDoubleSpinBox();
    m_spinRate = new QDoubleSpinBox();

    m_spinMin->setRange(-10000, 10000);
    m_spinMax->setRange(-10000, 10000);
    m_spinRate->setRange(0, 10000);

    m_spinMin->setEnabled(false);
    m_spinMax->setEnabled(false);
    m_spinRate->setEnabled(false);

    connect(m_chkMin, &QCheckBox::toggled, this, &SensorConfigDialog::toggleSpinBoxes);
    connect(m_chkMax, &QCheckBox::toggled, this, &SensorConfigDialog::toggleSpinBoxes);
    connect(m_chkRate, &QCheckBox::toggled, this, &SensorConfigDialog::toggleSpinBoxes);

    m_btnSave = new QPushButton("Сохранить");
    m_btnCancel = new QPushButton("Отмена");
    connect(m_btnSave, &QPushButton::clicked, this, &SensorConfigDialog::saveAndClose);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(m_chkMin, 0, 0);
    gridLayout->addWidget(m_spinMin, 0, 1);

    gridLayout->addWidget(m_chkMax, 1, 0);
    gridLayout->addWidget(m_spinMax, 1, 1);

    gridLayout->addWidget(m_chkRate, 2, 0);
    gridLayout->addWidget(m_spinRate, 2, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnSave);
    btnLayout->addWidget(m_btnCancel);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(btnLayout);

    SensorInfo settings = DatabaseManager::instance().getSensorSettings(m_sensorId);

    if (!qIsNaN(settings.minLimit)) {
        m_chkMin->setChecked(true);
        m_spinMin->setValue(settings.minLimit);
        m_spinMin->setEnabled(true);
    }

    if (!qIsNaN(settings.maxLimit)) {
        m_chkMax->setChecked(true);
        m_spinMax->setValue(settings.maxLimit);
        m_spinMax->setEnabled(true);
    }

    if (!qIsNaN(settings.maxRate)) {
        m_chkRate->setChecked(true);
        m_spinRate->setValue(settings.maxRate);
        m_spinRate->setEnabled(true);
    }
}

void SensorConfigDialog::toggleSpinBoxes() {
    m_spinMin->setEnabled(m_chkMin->isChecked());
    m_spinMax->setEnabled(m_chkMax->isChecked());
    m_spinRate->setEnabled(m_chkRate->isChecked());
}

void SensorConfigDialog::saveAndClose() {
    QVariant minLimit = m_chkMin->isChecked() ? m_spinMin->value() : QVariant();
    QVariant maxLimit = m_chkMax->isChecked() ? m_spinMax->value() : QVariant();
    QVariant maxRate = m_chkRate->isChecked() ? m_spinRate->value() : QVariant();

    DatabaseManager::instance().updateSensorThresholds(m_sensorId, minLimit, maxLimit, maxRate); //[cite: 15, 16]

    accept();
}