#include "../include/Dialogs.h"
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>


PolicyDialog::PolicyDialog(Mode mode, QWidget *parent)
    : QDialog(parent), m_mode(mode)
{
    setWindowTitle(m_mode == Create ? "Создать политику" : "Редактировать политику");
    setMinimumWidth(500);
    setupUI();
}

void PolicyDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout для полей ввода
    QFormLayout *formLayout = new QFormLayout();

    // Название политики
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Введите название политики");
    m_nameEdit->setMaxLength(100);
    formLayout->addRow("Название:", m_nameEdit);

    // Описание
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setPlaceholderText("Описание политики (необязательно)");
    formLayout->addRow("Описание:", m_descriptionEdit);

    // Регулярное выражение
    QGroupBox *patternGroup = new QGroupBox("Регулярное выражение для поиска", this);
    QVBoxLayout *patternLayout = new QVBoxLayout(patternGroup);

    m_patternEdit = new QTextEdit(patternGroup);
    m_patternEdit->setMaximumHeight(60);
    m_patternEdit->setPlaceholderText("Например: \\b\\d{4}[-\\s]?\\d{4}[-\\s]?\\d{4}[-\\s]?\\d{4}\\b");

    QHBoxLayout *patternButtonsLayout = new QHBoxLayout();
    m_testButton = new QPushButton("Проверить паттерн", patternGroup);
    QLabel *patternHint = new QLabel("Регулярные выражения C++", patternGroup);
    patternHint->setStyleSheet("color: gray; font-size: 10pt;");

    patternButtonsLayout->addWidget(m_testButton);
    patternButtonsLayout->addStretch();
    patternButtonsLayout->addWidget(patternHint);

    patternLayout->addWidget(m_patternEdit);
    patternLayout->addLayout(patternButtonsLayout);

    formLayout->addRow(patternGroup);

    // Уровень серьезности
    m_severityCombo = new QComboBox(this);
    m_severityCombo->addItems({"info", "low", "medium", "high", "critical"});
    m_severityCombo->setCurrentText("medium");
    formLayout->addRow("Уровень серьезности:", m_severityCombo);

    // Активность
    m_activeCheck = new QCheckBox("Политика активна", this);
    m_activeCheck->setChecked(true);
    formLayout->addRow("", m_activeCheck);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_saveButton = new QPushButton(m_mode == Create ? "Создать" : "Сохранить", this);
    m_saveButton->setDefault(true);
    m_saveButton->setMinimumWidth(100);

    m_cancelButton = new QPushButton("Отмена", this);
    m_cancelButton->setMinimumWidth(100);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Подключение сигналов
    connect(m_saveButton, &QPushButton::clicked, this, &PolicyDialog::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &PolicyDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &PolicyDialog::onTestPattern);
}

void PolicyDialog::setPolicyData(const DlpPolicy &policy)
{
    m_policy = policy;

    m_nameEdit->setText(policy.name);
    m_descriptionEdit->setText(policy.description);
    m_patternEdit->setText(policy.pattern);
    m_severityCombo->setCurrentText(policy.severity);
    m_activeCheck->setChecked(policy.isActive);
}

DlpPolicy PolicyDialog::getPolicyData() const
{
    DlpPolicy policy;

    policy.name = m_nameEdit->text().trimmed();
    policy.description = m_descriptionEdit->toPlainText().trimmed();
    policy.pattern = m_patternEdit->toPlainText().trimmed();
    policy.severity = m_severityCombo->currentText();
    policy.isActive = m_activeCheck->isChecked();

    if (m_mode == Edit) {
        policy.id = m_policy.id;
    }

    return policy;
}

void PolicyDialog::onSaveClicked()
{
    // Валидация данных
    QString name = m_nameEdit->text().trimmed();
    QString pattern = m_patternEdit->toPlainText().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Название политики не может быть пустым");
        m_nameEdit->setFocus();
        return;
    }

    if (pattern.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Регулярное выражение не может быть пустым");
        m_patternEdit->setFocus();
        return;
    }

    accept();
}

void PolicyDialog::onTestPattern()
{
    QString pattern = m_patternEdit->toPlainText().trimmed();

    if (pattern.isEmpty()) {
        QMessageBox::information(this, "Проверка паттерна", "Введите регулярное выражение для проверки");
        return;
    }

    QDialog testDialog(this);
    testDialog.setWindowTitle("Тестирование регулярного выражения");
    testDialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&testDialog);

    QLabel *infoLabel = new QLabel(&testDialog);
    infoLabel->setText("Введите текст для проверки регулярного выражения:");
    layout->addWidget(infoLabel);

    QTextEdit *testTextEdit = new QTextEdit(&testDialog);
    testTextEdit->setPlaceholderText("Пример: Номер карты: 4111-1111-1111-1111");
    testTextEdit->setMaximumHeight(100);
    layout->addWidget(testTextEdit);

    QPushButton *testBtn = new QPushButton("Проверить", &testDialog);
    layout->addWidget(testBtn);

    QLabel *resultLabel = new QLabel(&testDialog);
    resultLabel->setWordWrap(true);
    layout->addWidget(resultLabel);

    // Подключение кнопки проверки
    connect(testBtn, &QPushButton::clicked, [&testDialog, pattern, testTextEdit, resultLabel]() {
        QString testText = testTextEdit->toPlainText();

        if (testText.isEmpty()) {
            resultLabel->setText("<span style='color:orange'>Введите текст для проверки</span>");
            return;
        }

        try {
            QRegularExpression regex(pattern);

            if (!regex.isValid()) {
                resultLabel->setText(QString("<span style='color:red'>Ошибка в регулярном выражении: %1</span>")
                                    .arg(regex.errorString()));
                return;
            }

            QRegularExpressionMatch match = regex.match(testText);

            if (match.hasMatch()) {
                QString matchText = match.captured();
                QString message = QString("<span style='color:green'>Совпадение найдено!</span><br>"
                                         "Найдено: <b>%1</b><br>"
                                         "Позиция: %2-%3")
                                 .arg(matchText.toHtmlEscaped())
                                 .arg(match.capturedStart())
                                 .arg(match.capturedEnd());

                // Подсветка всех совпадений
                QString highlightedText = testText;
                QRegularExpressionMatchIterator it = regex.globalMatch(testText);
                int offset = 0;

                while (it.hasNext()) {
                    QRegularExpressionMatch m = it.next();
                    QString captured = m.captured();
                    int start = m.capturedStart() + offset;
                    int end = m.capturedEnd() + offset;

                    highlightedText.replace(start, end - start,
                                          QString("<span style='background-color:yellow;font-weight:bold'>%1</span>")
                                          .arg(captured));

                    offset += QString("<span style='background-color:yellow;font-weight:bold'></span>").length();
                }

                resultLabel->setText(message + "<br><br><b>Результат с подсветкой:</b><br>" + highlightedText);
            } else {
                resultLabel->setText("<span style='color:red'>Совпадений не найдено</span>");
            }
        } catch (...) {
            resultLabel->setText("<span style='color:red'>Непредвиденная ошибка при проверке паттерна</span>");
        }
    });

    testDialog.exec();
}