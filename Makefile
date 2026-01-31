AGENT_DIR = agent
GUI_DIR = gui
BACKEND_DIR = backend
BIN_DIR = bin
DOCKER_COMPOSE = docker-compose.yaml

.DEFAULT_GOAL := help

# Сборка всего проекта
all: backend agent gui

# Сборка и запуск backend с БД
backend: build-backend up

# Сборка агента
agent: build-agent

# Сборка GUI
gui: build-gui

# Сборка backend (Docker образ)
build-backend:
	@echo "Поднятие Docker образа.."
	docker build -t dlp-backend:latest ./backend

# Запуск сервисов через Docker Compose
up:
	@echo "Запуск сервера с БД..."
	docker-compose -f $(DOCKER_COMPOSE) up -d

# Остановка сервисов
down:
	@echo "Остановка сервисов..."
	docker-compose -f $(DOCKER_COMPOSE) down

# Перезапуск сервисов
restart: down up

# Сборка агента
build-agent:
	@echo "Сборка бинарника агента..."
	@mkdir -p $(BIN_DIR)
	cd $(AGENT_DIR) && \
	mkdir -p build && \
	cd build && \
	cmake .. && \
	make -j$(nproc)
	@cp $(AGENT_DIR)/build/DLP_Agent $(BIN_DIR)/dlp-agent
	@echo "Agent binary created at $(BIN_DIR)/dlp-agent"

# Сборка GUI
build-gui:
	@echo "Сборка приложения GUI..."
	@mkdir -p $(BIN_DIR)
	cd $(GUI_DIR) && \
	mkdir -p build && \
	cd build && \
	cmake .. && \
	make -j$(nproc)
	@cp $(GUI_DIR)/build/DLP_GUI $(BIN_DIR)/dlp-gui
	@echo "GUI binary created at $(BIN_DIR)/dlp-gui"

# Создание AppImage для GUI
appimage: build-gui
	@echo "Создание AppImage..."
	@if command -v linuxdeploy >/dev/null 2>&1; then \
		cd $(GUI_DIR)/build && \
		linuxdeploy --appdir AppDir -e dlp-gui --create-desktop-file \
		--icon-file=../resources/icons/dlp-icon.png --output appimage; \
		mv dlp-gui*.AppImage ../../$(BIN_DIR)/dlp-gui.AppImage; \
		echo "AppImage created at $(BIN_DIR)/dlp-gui.AppImage"; \
	else \
		echo "linuxdeploy not found. Install with: sudo snap install linuxdeploy --classic"; \
	fi

# Очистка сборок
clean:
	@echo "Cleaning builds..."
	rm -rf $(AGENT_DIR)/build $(GUI_DIR)/build $(BIN_DIR)
	docker-compose -f $(DOCKER_COMPOSE) down -v

# Полная пересборка
rebuild: clean all

# Логи сервера
logs:
	docker-compose -f $(DOCKER_COMPOSE) logs -f backend

# Статус сервисов
status:
	docker-compose -f $(DOCKER_COMPOSE) ps

# Установка зависимостей для разработки
dev-deps:
	@echo "Установка зависимостей..."
	# Для Ubuntu/Debian
	sudo apt-get update
	sudo apt-get install -y cmake build-essential qt6-base-dev libqt6charts6-dev qt6-tools-dev qt6-l10n-tools
	# Для сборки AppImage
	sudo snap install linuxdeploy --classic

# Проверка конфигурации
check:
	@echo "Проверка структуры проекта..."
	@[ -f "$(DOCKER_COMPOSE)" ] && echo "✓ docker-compose.yaml" || echo "✗ docker-compose.yaml"
	@[ -d "$(AGENT_DIR)" ] && echo "✓ Директория агента" || echo "✗ Директория агента"
	@[ -d "$(GUI_DIR)" ] && echo "✓ Директория GUI" || echo "✗ Директория GUI"
	@[ -d "$(BACKEND_DIR)" ] && echo "✓ Директория сервера" || echo "✗ Директория сервера"

# Справка
help:
	@echo "Вспомогательные команды:"
	@echo "  make all           - Собрать каждый компонент (backend, agent, gui)"
	@echo "  make backend       - Собрать и запустить сервер с БД"
	@echo "  make agent         - Собрать бинарник агента"
	@echo "  make gui           - Собрать бинарник GUI"
	@echo "  make appimage      - Создать AppImage для GUI"
	@echo "  make up           - Запустить сервисы Backend"
	@echo "  make down         - Остановить сервисы Backend"
	@echo "  make restart      - Перезапустить сервисы Backend"
	@echo "  make clean        - Очистить собранные компоненты и прекратить работу"
	@echo "  make rebuild      - Полная пересборка проекта"
	@echo "  make logs         - Посмотреть логи сервера"
	@echo "  make status       - Проверить состояние сервера"
	@echo "  make dev-deps     - Установить необходимые зависимости"
	@echo "  make check        - Проверить корректность структуры проекта"

.PHONY: all backend agent gui build-backend up down restart build-agent build-gui appimage clean rebuild logs status dev-deps check help
