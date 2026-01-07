package utils

import (
	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"os"
	"time"
)

// InitLogger - настройка глобального логгера
func InitLogger() zerolog.Logger {
	output := zerolog.ConsoleWriter{
		Out:        os.Stdout,
		TimeFormat: time.RFC3339,
		NoColor:    false,
	}

	logger := zerolog.New(output).
		With().
		Timestamp().
		Caller().
		Logger()

	// Уровень логирования
	zerolog.SetGlobalLevel(zerolog.InfoLevel)
	log.Logger = logger

	return logger
}

// GetLogger - логгер с дополнительным контекстом
func GetLogger() zerolog.Logger {
	return log.Logger
}

// WithRequestID - добавление ID запроса к логгеру
func WithRequestID(logger zerolog.Logger, requestID string) zerolog.Logger {
	return logger.With().Str("request_id", requestID).Logger()
}

// WithComponent - добавление имени компонента к логгеру
func WithComponent(logger zerolog.Logger, component string) zerolog.Logger {
	return logger.With().Str("component", component).Logger()
}
