package main

import (
	"DLP_Server/config"
	"DLP_Server/handlers"
	"DLP_Server/middleware"
	"DLP_Server/store"
	"DLP_Server/utils"
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/go-chi/chi/v5"
	chimiddleware "github.com/go-chi/chi/v5/middleware"
	"github.com/rs/zerolog/log"
)

func main() {
	// Инициализация логгера
	logger := utils.InitLogger()
	log.Info().Msg("Запуск DLP сервера...")

	// Загрузка конфигурации
	cfg, err := config.Load()
	if err != nil {
		log.Fatal().Err(err).Msg("Ошибка загрузки конфигурации")
	}
	log.Info().
		Str("environment", cfg.Environment).
		Int("port", cfg.ServerPort).
		Msg("Конфигурация загружена")

	// Подключение к базе данных
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	dbStore, err := store.NewGormStore(ctx, cfg)
	if err != nil {
		log.Fatal().Err(err).Msg("Ошибка подключения к базе данных через GORM")
	}
	defer dbStore.Close()
	log.Info().Msg("Подключение к PostgreSQL через GORM установлено")

	handler := handlers.NewHandler(*dbStore, logger)
	router := chi.NewRouter()

	router.Use(chimiddleware.RequestID)
	router.Use(chimiddleware.RealIP)
	router.Use(chimiddleware.Recoverer)
	router.Use(chimiddleware.Timeout(cfg.ServerRequestTimeout))
	router.Use(middleware.CORS())
	router.Use(middleware.RequestLogger(logger))

	// Health check
	router.Get("/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, `{"status":"ok","service":"dlp-server","timestamp":"%s"}`,
			time.Now().Format(time.RFC3339))
	})

	// API
	router.Route("/api/v1", func(r chi.Router) {
		// Агенты
		r.Route("/agents", func(r chi.Router) {
			r.Post("/register", handler.RegisterAgent)
			r.Get("/", handler.GetAgents)
			r.Put("/{id}/heartbeat", handler.UpdateAgentHeartbeat)
			r.Delete("/{id}", handler.DeleteAgent)
		})

		// Политики DLP
		r.Route("/policies", func(r chi.Router) {
			r.Get("/", handler.GetPolicies)
			r.Get("/agent", handler.GetPoliciesForAgent)
			r.Post("/", handler.CreatePolicy)
			r.Put("/{id}", handler.UpdatePolicy)
			r.Delete("/{id}", handler.DeletePolicy)
		})

		// События
		r.Route("/events", func(r chi.Router) {
			r.Post("/", handler.HandleEvent)
			r.Get("/", handler.GetEvents)
		})

		// Инциденты
		r.Route("/incidents", func(r chi.Router) {
			r.Get("/", handler.GetIncidents)
			r.Get("/stats", handler.GetIncidentStats)
			r.Get("/{id}", handler.GetIncident)
			r.Put("/{id}/status", handler.UpdateIncidentStatus)
		})
	})

	// Настройка HTTP сервера
	server := &http.Server{
		Addr:         fmt.Sprintf(":%d", cfg.ServerPort),
		Handler:      router,
		ReadTimeout:  cfg.ServerReadTimeout,
		WriteTimeout: cfg.ServerWriteTimeout,
		IdleTimeout:  cfg.ServerIdleTimeout,
	}

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		log.Info().Int("port", cfg.ServerPort).Msg("Сервер запущен")
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatal().Err(err).Msg("Ошибка запуска сервера")
		}
	}()

	<-quit
	log.Info().Msg("Получен сигнал завершения...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer shutdownCancel()

	if err := server.Shutdown(shutdownCtx); err != nil {
		log.Error().Err(err).Msg("Ошибка при graceful shutdown")
	}
	log.Info().Msg("Сервер корректно остановлен")
}
