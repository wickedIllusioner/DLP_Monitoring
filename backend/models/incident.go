package models

import (
	"time"
)

// Incident - DLP-инцидент
type Incident struct {
	ID             int64      `json:"id" gorm:"primaryKey;autoIncrement"`
	EventID        int64      `json:"event_id" gorm:"not null"`
	PolicyID       *int64     `json:"policy_id,omitempty" gorm:"index"`
	Severity       string     `json:"severity" gorm:"size:20;not null"`
	Status         string     `json:"status" gorm:"size:20;default:'new';check status IN ('new', 'investigating', 'resolved', 'false_positive')"`
	MatchedContent string     `json:"matched_content" gorm:"type:text"`
	CreatedAt      time.Time  `json:"created_at" gorm:"autoCreateTime"`
	ResolvedAt     *time.Time `json:"resolved_at,omitempty" db:"resolved_at"`
	ResolvedBy     *string    `json:"resolved_by,omitempty" gorm:"size:100"`
	//DeletedAt      gorm.DeletedAt `json:"-" gorm:"index"`

	Event  *Event  `gorm:"foreignKey:EventID;references:ID"`
	Policy *Policy `gorm:"foreignKey:PolicyID"`
}

// IncidentCreate - запрос создания инцидента
type IncidentCreate struct {
	EventID        int64  `json:"event_id" validate:"required"`
	PolicyID       *int64 `json:"policy_id,omitempty"`
	Severity       string `json:"severity" validate:"required"`
	MatchedContent string `json:"matched_content" validate:"required"`
}

// IncidentUpdate - запрос обновления инцидента
type IncidentUpdate struct {
	Status     string `json:"status" validate:"required"`
	ResolvedBy string `json:"resolved_by,omitempty"`
}

// IncidentStats - статистика по инцидентам
type IncidentStats struct {
	TotalIncidents int64 `json:"total_incidents"`
	NewIncidents   int64 `json:"new_incidents"`
	Investigating  int64 `json:"investigating"`
	Resolved       int64 `json:"resolved"`
	FalsePositive  int64 `json:"false_positive"`

	SeverityStats   []SeverityCount `json:"severity_stats"`
	RecentIncidents []Incident      `json:"recent_incidents"`
}

// SeverityCount - кол-во инцидентов по уровням серьезности
type SeverityCount struct {
	Severity string `json:"severity"`
	Count    int64  `json:"count"`
}
