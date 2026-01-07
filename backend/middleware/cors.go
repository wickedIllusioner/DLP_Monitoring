package middleware

import (
	"github.com/go-chi/cors"
	"net/http"
)

// CORS - middleware для обработки кросс-доменных запросов
func CORS() func(next http.Handler) http.Handler {
	// Настройки CORS для разработки
	corsMiddleware := cors.New(cors.Options{
		AllowedOrigins: []string{"*"},

		AllowedMethods: []string{
			"GET",
			"POST",
			"PUT",
			"DELETE",
			"OPTIONS", // Preflight запросы
		},

		AllowedHeaders: []string{
			"Accept",
			"Content-Type",
			"Authorization",
			"X-Requested-With",
		},

		ExposedHeaders: []string{
			"Content-Length",
			"X-Total-Count",
		},

		AllowCredentials: false,

		// Кэширование preflight запросов
		MaxAge: 300,
	})

	return corsMiddleware.Handler
}
