package models

import (
	"time"
)

// Policy - DLP-политика
type Policy struct {
	ID          int64     `json:"id" gorm:"primaryKey;autoIncrement"`
	Name        string    `json:"name" gorm:"size:100;not null"`
	Description string    `json:"description" gorm:"type:text"`
	Pattern     string    `json:"pattern" gorm:"type:text;not null"`
	Severity    string    `json:"severity" gorm:"size:20;check:severity IN ('info', 'low', 'medium', 'high', 'critical');not null"`
	IsActive    bool      `json:"is_active" gorm:"default:true"`
	CreatedAt   time.Time `json:"created_at" gorm:"autoCreateTime"`
	UpdatedAt   time.Time `json:"updated_at" gorm:"autoUpdateTime"`

	Incidents []Incident `gorm:"foreignKey:PolicyID"`
}

// PolicyCreate - запрос создания политики
type PolicyCreate struct {
	Name        string `json:"name" validate:"required"`
	Description string `json:"description"`
	Pattern     string `json:"pattern" validate:"required"`
	Severity    string `json:"severity" validate:"required"`
	IsActive    bool   `json:"is_active"`
}

// PolicyUpdate - запрос обновления политики
type PolicyUpdate struct {
	Name        string `json:"name"`
	Description string `json:"description"`
	Pattern     string `json:"pattern"`
	Severity    string `json:"severity"`
	IsActive    *bool  `json:"is_active"`
}
