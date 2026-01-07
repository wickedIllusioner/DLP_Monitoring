package models

import (
	"time"
)

// Event - событие от агента
type Event struct {
	ID            int64     `json:"id" gorm:"primaryKey;autoIncrement"`
	AgentID       int64     `json:"agent_id" gorm:"not null;index"`
	FilePath      string    `json:"file_path" gorm:"type:text;not null"`
	FileName      string    `json:"file_name" gorm:"size:255;not null"`
	EventType     string    `json:"event_type" gorm:"size:50;not null"`
	FileSize      *int64    `json:"file_size,omitempty"`
	ContentSample string    `json:"content_sample" gorm:"type:text"`
	DetectedAt    time.Time `json:"detected_at" gorm:"default:CURRENT_TIMESTAMP"`
	CreatedAt     time.Time `json:"created_at" gorm:"autoCreateTime"`
	//DeletedAt     gorm.DeletedAt `json:"-" gorm:"index"`

	Agent    Agent     `gorm:"foreignKey:AgentID;references:ID"`
	Incident *Incident `gorm:"foreignKey:EventID"`
}

// EventCreate - запрос создания события
type EventCreate struct {
	AgentID       string    `json:"agent_id" validate:"required"`
	FilePath      string    `json:"file_path" validate:"required"`
	FileName      string    `json:"file_name" validate:"required"`
	EventType     string    `json:"event_type" validate:"required"`
	FileSize      *int64    `json:"file_size,omitempty"`
	ContentSample string    `json:"content_sample" validate:"required"`
	DetectedAt    time.Time `json:"detected_at"`

	IsViolation   bool    `json:"is_violation"`
	ViolationType *string `json:"violation_type,omitempty"`
	MatchedPolicy *string `json:"matched_policy,omitempty"`
	Severity      *string `json:"severity,omitempty"`
}
