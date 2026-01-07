-- database/init.sql

-- Таблица агентов
CREATE TABLE IF NOT EXISTS agents (
    id SERIAL PRIMARY KEY,
    agent_uuid VARCHAR(100) UNIQUE NOT NULL,
    hostname VARCHAR(255) NOT NULL,
    ip_address VARCHAR(45),
    os_info TEXT,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Таблица политик DLP
CREATE TABLE IF NOT EXISTS policies (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    pattern TEXT NOT NULL,
    severity VARCHAR(20) NOT NULL CHECK (severity IN ('info', 'low', 'medium', 'high', 'critical')),
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Таблица событий
CREATE TABLE IF NOT EXISTS events (
    id SERIAL PRIMARY KEY,
    agent_id INTEGER REFERENCES agents(id) ON DELETE SET NULL,
    file_path TEXT NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    event_type VARCHAR(50) NOT NULL,
    file_size BIGINT,
    content_sample TEXT,
    detected_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Таблица инцидентов
CREATE TABLE IF NOT EXISTS incidents (
    id SERIAL PRIMARY KEY,
    event_id INTEGER REFERENCES events(id) ON DELETE CASCADE,
    policy_id INTEGER REFERENCES policies(id) ON DELETE SET NULL,
    severity VARCHAR(20) NOT NULL,
    status VARCHAR(20) DEFAULT 'new' CHECK (status IN ('new', 'investigating', 'resolved', 'false_positive')),
    matched_content TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    resolved_at TIMESTAMP,
    resolved_by VARCHAR(100)
);

CREATE INDEX idx_incidents_created_at ON incidents(created_at);
CREATE INDEX idx_incidents_status ON incidents(status);
CREATE INDEX idx_incidents_severity ON incidents(severity);
CREATE INDEX idx_events_detected_at ON events(detected_at);
CREATE INDEX idx_agents_last_seen ON agents(last_seen);

-- Триггер для обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_agents_updated_at BEFORE UPDATE ON agents
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_policies_updated_at BEFORE UPDATE ON policies
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();


-- Вставка базовых DLP политик безопасности
INSERT INTO policies (name, description, pattern, severity, is_active) VALUES
-- 1. Кредитные карты
('Кредитные карты (Visa)', 'Обнаружение номеров Visa карт (начинаются с 4)',
 '\b4[0-9]{12}(?:[0-9]{3})?\b', 'high', true),

('Кредитные карты (MasterCard)', 'Обнаружение номеров MasterCard (начинаются с 51-55)',
 '\b5[1-5][0-9]{14}\b', 'high', true),

('Кредитные карты (American Express)', 'Обнаружение номеров American Express (начинаются с 34, 37)',
 '\b3[47][0-9]{13}\b', 'high', true),

-- 2. Российские паспорта
('Российский паспорт (новый)', 'Обнаружение номеров российских паспортов (формат: 1234 567890)',
 '\b\d{4}\s+\d{6}\b', 'critical', true),

('Российский паспорт (старый)', 'Обнаружение номеров российских паспортов (формат с №)',
 '\b\d{4}\s*№\s*\d{6}\b', 'critical', true),

-- 3. Email адреса
('Email адреса', 'Обнаружение email адресов',
 '\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b', 'medium', true),

('Корпоративные email', 'Обнаружение email с доменами компаний',
 '\b[A-Za-z0-9._%+-]+@(?:company|corp|enterprise|business)\.(?:com|ru|net|org)\b', 'high', true),

-- 4. Российские телефонные номера
('Российские мобильные', 'Обнаружение российских мобильных номеров (+7 9XX XXX-XX-XX)',
 '\+7\s?\(?9\d{2}\)?\s?\d{3}[-]?\d{2}[-]?\d{2}\b', 'medium', true),

('Российские городские', 'Обнаружение российских городских номеров',
 '\+7\s?\(?\d{3,5}\)?\s?\d{1,3}[-]?\d{2}[-]?\d{2}\b', 'low', true),

-- 5. Гриф секретности
('Гриф секретности (рус)', 'Обнаружение грифа секретности на русском',
 '\b(?:СЕКРЕТНО|КОНФИДЕНЦИАЛЬНО|ДСП|ОВ|ОСОБОЙ\s+ВАЖНОСТИ)\b', 'high', true),

('Гриф секретности (eng)', 'Обнаружение грифа секретности на английском',
 '\b(?:CONFIDENTIAL|SECRET|TOP[\s-]?SECRET|RESTRICTED|CLASSIFIED)\b', 'high', true),

-- 6. Банковские реквизиты
('Номер счета (РФ)', 'Обнаружение номеров банковских счетов (РФ)',
 '\b\d{20}\b', 'high', true),

('БИК банка', 'Обнаружение БИК российских банков',
 '\b\d{9}\b', 'medium', true),

('Корреспондентский счет', 'Обнаружение корреспондентских счетов',
 '\b301\d{17}\b', 'high', true),

-- 7. ИНН организаций и физлиц
('ИНН организации (10 цифр)', 'Обнаружение ИНН организаций',
 '\b\d{10}\b', 'medium', true),

('ИНН физлица (12 цифр)', 'Обнаружение ИНН физических лиц',
 '\b\d{12}\b', 'medium', true),

-- 8. КПП организаций
('КПП организации', 'Обнаружение КПП организаций',
 '\b\d{9}\b', 'medium', true),

-- 9. ОГРН/ОГРНИП
('ОГРН организации', 'Обнаружение ОГРН (13 цифр)',
 '\b\d{13}\b', 'medium', true),

('ОГРНИП', 'Обнаружение ОГРНИП (15 цифр)',
 '\b\d{15}\b', 'medium', true),

-- 10. СНИЛС
('СНИЛС', 'Обнаружение номеров СНИЛС',
 '\b\d{3}-\d{3}-\d{3}\s\d{2}\b', 'critical', true),

-- 11. Пароли и токены
('Пароли в тексте', 'Обнаружение слов "пароль", "password" с значениями',
 '(?i)\b(?:пароль|password|pwd|pass)\s*[:=]\s*[^\s]{6,}\b', 'high', true),

('API ключи', 'Обнаружение API ключей и токенов',
 '\b(?:sk|pk|AKIA|SG\.|Bearer\s)[a-zA-Z0-9_\-\.]{20,}\b', 'critical', true),

-- 12. Коды доступа
('Одноразовые коды', 'Обнаружение 6-значных кодов (SMS, 2FA)',
 '\b\d{6}\b', 'medium', true),

-- 13. Социальные сети
('VK ID', 'Обнаружение ID профилей VK',
 '\b(?:vk\.com/|id)\d{1,10}\b', 'low', true),

('Telegram', 'Обнаружение ссылок на Telegram',
 '\b(?:t\.me/|telegram\.me/|@)[A-Za-z0-9_]{5,32}\b', 'low', true),

-- 14. Криптокошельки
('Bitcoin адрес', 'Обнаружение Bitcoin адресов',
 '\b[13][a-km-zA-HJ-NP-Z1-9]{25,34}\b', 'high', true),

('Ethereum адрес', 'Обнаружение Ethereum адресов',
 '\b0x[a-fA-F0-9]{40}\b', 'high', true),

-- 15. Номера документов
('Водительское удостоверение', 'Обнаружение номеров водительских прав',
 '\b\d{2}\s?\d{2}\s?\d{6}\b', 'medium', true),

('Загранпаспорт', 'Обнаружение номеров загранпаспортов',
 '\b\d{2}\s?\d{7}\b', 'medium', true),

-- 16. Персональные данные
('ФИО с инициалами', 'Обнаружение ФИО в русском формате',
 '\b[А-ЯЁ][а-яё]+\s+[А-ЯЁ]\.\s*[А-ЯЁ]\.\b', 'low', true),

('Дата рождения', 'Обнаружение дат рождения',
 '\b\d{2}\.\d{2}\.\d{4}\b', 'low', true),

-- 17. Медицинские данные
('Номер полиса ОМС', 'Обнаружение номеров полисов ОМС',
 '\b\d{16}\b', 'critical', true),

('Номер истории болезни', 'Обнаружение номеров медицинских карт',
 '\b\d{6,10}\b', 'medium', true),

-- 18. Геолокационные данные
('Координаты GPS', 'Обнаружение GPS координат',
 '\b\d{1,3}\.\d{4,6},\s*\d{1,3}\.\d{4,6}\b', 'low', true),

('Домашний адрес', 'Обнаружение адресов (ул., дом, кв.)',
 '(?i)\b(?:ул|улица|проспект|пр|бульвар|б-р|переулок|пер)\.?\s+[^,]+,\s*(?:д|дом)\.?\s*\d+[^,]*,\s*(?:кв|квартира)\.?\s*\d+\b', 'low', true),

-- 19. Финансовые данные
('Сумма в рублях', 'Обнаружение денежных сумм с рублями',
 '\b\d{1,3}(?:[ ,]\d{3})*(?:\.\d{2})?\s*(?:руб|р\.|RUB)\b', 'medium', true),

('Сумма в долларах', 'Обнаружение денежных сумм с долларами',
 '\b\d{1,3}(?:[ ,]\d{3})*(?:\.\d{2})?\s*(?:usd|\$|долл)\b', 'medium', true),

-- 20. Служебные метки
('Внутренняя документация', 'Обнаружение меток внутренних документов',
 '\b(?:internal|внутренний|for office use only|служебная записка)\b', 'medium', true),

('Номер договора', 'Обнаружение номеров договоров и контрактов',
 '\b(?:договор|контракт|contract|agreement)\s*№?\s*\d{1,5}(?:[/\-]\d{2,4})?\b', 'medium', true);