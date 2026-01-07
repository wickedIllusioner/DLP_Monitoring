package config

import (
	"fmt"
	"github.com/joho/godotenv"
	"gorm.io/gorm/logger"
	"os"
	"strconv"
	"strings"
	"time"
)

type Config struct {
	// Сервер
	Environment          string
	ServerPort           int
	ServerReadTimeout    time.Duration
	ServerWriteTimeout   time.Duration
	ServerIdleTimeout    time.Duration
	ServerRequestTimeout time.Duration
	ServerTLSEnabled     bool
	ServerTLSCertPath    string
	ServerTLSKeyPath     string

	// БД
	DBHost     string
	DBPort     int
	DBName     string
	DBUser     string
	DBPassword string
	DBSSLMode  string

	// CORS
	CORSEnabled          bool
	CORSAllowedOrigins   []string
	CORSAllowCredentials bool
	CORSMaxAge           int

	// Логи
	LogLevel string

	// GORM
	GORMLogLevel logger.LogLevel

	// Настройки DLP
	EventRetentionDays  int
	IncidentAutoResolve bool
	ScanBatchSize       int
}

func Load() (*Config, error) {
	_ = godotenv.Load()

	cfg := &Config{
		Environment:          getEnv("ENVIRONMENT", "development"),
		ServerPort:           getEnvAsInt("SERVER_PORT", 8080),
		ServerReadTimeout:    getEnvAsDuration("SERVER_READ_TIMEOUT", 15*time.Second),
		ServerWriteTimeout:   getEnvAsDuration("SERVER_WRITE_TIMEOUT", 15*time.Second),
		ServerIdleTimeout:    getEnvAsDuration("SERVER_IDLE_TIMEOUT", 60*time.Second),
		ServerRequestTimeout: getEnvAsDuration("SERVER_REQUEST_TIMEOUT", 60*time.Second),
		ServerTLSEnabled:     getEnvAsBool("SERVER_TLS_ENABLED", false),
		ServerTLSCertPath:    getEnv("SERVER_TLS_CERT_PATH", ""),
		ServerTLSKeyPath:     getEnv("SERVER_TLS_KEY_PATH", ""),

		DBHost:     getEnv("DB_HOST", "localhost"),
		DBPort:     getEnvAsInt("DB_PORT", 5432),
		DBName:     getEnv("DB_NAME", "dlp_system"),
		DBUser:     getEnv("DB_USER", "dlp_admin"),
		DBPassword: getEnv("DB_PASSWORD", "dlp_pass123"),
		DBSSLMode:  getEnv("DB_SSL_MODE", "disable"),

		GORMLogLevel: getGORMLogLevel(),

		LogLevel: getEnv("LOG_LEVEL", "info"),

		CORSEnabled:          getEnvAsBool("CORS_ENABLED", true),
		CORSAllowedOrigins:   getEnvAsSlice("CORS_ALLOWED_ORIGINS", []string{"*"}, ","),
		CORSAllowCredentials: getEnvAsBool("CORS_ALLOW_CREDENTIALS", false),
		CORSMaxAge:           getEnvAsInt("CORS_MAX_AGE", 300),

		EventRetentionDays:  getEnvAsInt("EVENT_RETENTION_DAYS", 30),
		IncidentAutoResolve: getEnvAsBool("INCIDENT_AUTO_RESOLVE", false),
		ScanBatchSize:       getEnvAsInt("SCAN_BATCH_SIZE", 100),
	}

	if err := cfg.validate(); err != nil {
		return nil, err
	}

	return cfg, nil
}

func (c *Config) DatabaseURL() string {
	return fmt.Sprintf("postgres://%s:%s@%s:%d/%s?sslmode=%s",
		c.DBUser, c.DBPassword, c.DBHost, c.DBPort, c.DBName, c.DBSSLMode)
}

func (c *Config) validate() error {
	if c.ServerPort < 1 || c.ServerPort > 65535 {
		return fmt.Errorf("неверный порт сервера: %d", c.ServerPort)
	}

	if c.DBHost == "" {
		return fmt.Errorf("не указан хост базы данных")
	}

	if c.DBName == "" {
		return fmt.Errorf("не указано имя базы данных")
	}

	if c.ServerTLSEnabled && (c.ServerTLSCertPath == "" || c.ServerTLSKeyPath == "") {
		return fmt.Errorf("для TLS необходимо указать пути к сертификату и ключу")
	}

	return nil
}

func getEnv(key, defaultValue string) string {
	if value, exists := os.LookupEnv(key); exists {
		return value
	}
	return defaultValue
}

func getEnvAsInt(key string, defaultValue int) int {
	if value, exists := os.LookupEnv(key); exists {
		if intValue, err := strconv.Atoi(value); err == nil {
			return intValue
		}
	}
	return defaultValue
}

func getEnvAsBool(key string, defaultValue bool) bool {
	if value, exists := os.LookupEnv(key); exists {
		if boolValue, err := strconv.ParseBool(value); err == nil {
			return boolValue
		}
	}
	return defaultValue
}

func getEnvAsDuration(key string, defaultValue time.Duration) time.Duration {
	if value, exists := os.LookupEnv(key); exists {
		if duration, err := time.ParseDuration(value); err == nil {
			return duration
		}
	}
	return defaultValue
}

func getEnvAsSlice(key string, defaultValue []string, sep string) []string {
	if value, exists := os.LookupEnv(key); exists {
		return strings.Split(value, sep)
	}
	return defaultValue
}

func getGORMLogLevel() logger.LogLevel {
	level := getEnv("GORM_LOG_LEVEL", "warn")
	switch strings.ToLower(level) {
	case "silent":
		return logger.Silent
	case "error":
		return logger.Error
	case "warn":
		return logger.Warn
	case "info":
		return logger.Info
	default:
		return logger.Warn
	}
}
