#include "ConfigureDeviceDialog.h"
#include <QDialogButtonBox>

ConfigureDeviceDialog::ConfigureDeviceDialog(int deviceId, QWidget* parent)
    : QDialog(parent), m_deviceId(deviceId) {

    setupUI();
    loadData();
    setWindowTitle("Настройка устройства");
    resize(350, 200); // Сделали окно компактнее
}

void ConfigureDeviceDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();

    m_idLabel = new QLabel(this);
    m_idLabel->setStyleSheet("color: gray;"); // Визуально выделяем, что это только для чтения

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Введите название (напр. Лампа)");

    m_roomCombo = new QComboBox(this);
    m_roomCombo->setEditable(true);
    m_roomCombo->setInsertPolicy(QComboBox::NoInsert);
    m_roomCombo->setPlaceholderText("Выберите или введите новую комнату...");

    formLayout->addRow("Уникальный ID:", m_idLabel);
    formLayout->addRow("Имя устройства:", m_nameEdit);
    formLayout->addRow("Комната:", m_roomCombo);

    mainLayout->addLayout(formLayout);

    // Стандартные кнопки OK/Cancel
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigureDeviceDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

void ConfigureDeviceDialog::loadData() {
    auto rooms = DatabaseManager::instance().getRooms();
    for (const auto& room : rooms) {
        m_roomCombo->addItem(room.name, room.id);
    }

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("SELECT name, unique_id, room_id FROM devices WHERE id = ?");
    q.addBindValue(m_deviceId);

    if (q.exec() && q.next()) {
        m_nameEdit->setText(q.value(0).toString());
        m_idLabel->setText(q.value(1).toString());
        int currentRoomId = q.value(2).toInt();

        int index = m_roomCombo->findData(currentRoomId);
        if (index != -1) m_roomCombo->setCurrentIndex(index);
    }
}

void ConfigureDeviceDialog::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    QString roomName = m_roomCombo->currentText().trimmed();

    if (name.isEmpty() || roomName.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Имя и комната не могут быть пустыми.");
        return;
    }

    int roomId = -1;
    int index = m_roomCombo->findText(roomName);

    // Если комнаты нет в списке — создаем её
    if (index != -1 && m_roomCombo->itemData(index).isValid()) {
        roomId = m_roomCombo->itemData(index).toInt();
    }
    else {
        if (DatabaseManager::instance().createRoom(roomName)) {
            // Получаем ID только что созданной комнаты
            QSqlQuery q("SELECT LAST_INSERT_ID()", DatabaseManager::instance().database());
            if (q.next()) roomId = q.value(0).toInt();
        }
    }

    if (DatabaseManager::instance().updateDeviceConfig(m_deviceId, name, roomId)) {
        accept(); // Закрываем диалог с успехом
    }
    else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить конфигурацию.");
    }
}