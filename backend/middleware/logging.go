package middleware

import (
	"github.com/go-chi/chi/v5/middleware"
	"github.com/rs/zerolog"
	"net/http"
	"time"
)

// RequestLogger - middleware для логирования HTTP запросов
func RequestLogger(logger zerolog.Logger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {

			start := time.Now()
			requestID := middleware.GetReqID(r.Context())

			// Обертка для ResponseWriter для получения статуса ответа
			ww := middleware.NewWrapResponseWriter(w, r.ProtoMajor)

			next.ServeHTTP(ww, r)
			duration := time.Since(start)

			logEntry := logger.Info().
				Str("method", r.Method).
				Str("path", r.URL.Path).
				Str("remote_addr", r.RemoteAddr).
				Int("status", ww.Status()).
				Dur("duration_ms", duration).
				Int("bytes_written", ww.BytesWritten())

			if requestID != "" {
				logEntry = logEntry.Str("request_id", requestID)
			}

			if userAgent := r.UserAgent(); userAgent != "" {
				logEntry = logEntry.Str("user_agent", userAgent)
			}

			if ww.Status() >= 400 {
				logEntry = logger.Error().
					Str("method", r.Method).
					Str("path", r.URL.Path).
					Str("remote_addr", r.RemoteAddr).
					Int("status", ww.Status()).
					Dur("duration_ms", duration)
			}
			logEntry.Msg("HTTP request processed")
		}
		return http.HandlerFunc(fn)
	}
}
