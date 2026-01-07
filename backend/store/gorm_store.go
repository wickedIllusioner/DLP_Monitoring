package store

import (
	"DLP_Server/config"
	"context"
	"fmt"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"
	"time"
)

type GormStore struct {
	db *gorm.DB
}

func NewGormStore(ctx context.Context, cfg *config.Config) (*GormStore, error) {
	dsn := fmt.Sprintf("host=%s user=%s password=%s dbname=%s port=%d sslmode=%s",
		cfg.DBHost, cfg.DBUser, cfg.DBPassword, cfg.DBName, cfg.DBPort, cfg.DBSSLMode)

	gormConfig := &gorm.Config{
		Logger: logger.Default.LogMode(cfg.GORMLogLevel),
		NowFunc: func() time.Time {
			return time.Now().UTC()
		},
		DisableForeignKeyConstraintWhenMigrating: true,
	}

	db, err := gorm.Open(postgres.Open(dsn), gormConfig)
	if err != nil {
		return nil, fmt.Errorf("Ошибка подключения к БД: %w", err)
	}

	//err = db.AutoMigrate(
	//	&models.Policy{},
	//	&models.Agent{},
	//	&models.Event{},
	//	&models.Incident{})
	//if err != nil {
	//	return nil, fmt.Errorf("Ошибка автомиграции: %w", err)
	//}

	sqlDB, err := db.DB()
	if err != nil {
		return nil, err
	}

	sqlDB.SetMaxOpenConns(10)
	sqlDB.SetMaxIdleConns(5)
	sqlDB.SetConnMaxLifetime(time.Hour)

	if err := sqlDB.PingContext(ctx); err != nil {
		return nil, fmt.Errorf("Ошибка проверки подключения к БД: %w", err)
	}

	return &GormStore{db: db}, nil
}

// Close - закрытие соединения с БД
func (s *GormStore) Close() error {
	sqlDB, err := s.db.DB()
	if err != nil {
		return err
	}
	return sqlDB.Close()
}
