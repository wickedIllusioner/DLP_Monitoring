package models

import (
	"time"
)

type Agent struct {
	ID        int64     `json:"id" gorm:"primaryKey;autoIncrement"`
	AgentUUID string    `json:"agent_id" gorm:"size:100;uniqueIndex;not null"`
	Hostname  string    `json:"hostname" gorm:"size:255;not null"`
	IPAddress string    `json:"ip_address" gorm:"size:45"`
	OSInfo    string    `json:"os_info" `
	LastSeen  time.Time `json:"last_seen" gorm:"default:CURRENT_TIMESTAMP"`
	CreatedAt time.Time `json:"created_at" gorm:"autoCreateTime"`
	UpdatedAt time.Time `json:"updated_at" gorm:"autoUpdateTime"`
	//DeletedAt gorm.DeletedAt `json:"-" gorm:"index"`

	Events []Event `gorm:"foreignKey:AgentID;references:ID"`
}

// AgentRegistration - запрос на регистрацию агента
type AgentRegistration struct {
	AgentID   string `json:"agent_id" validate:"required"`
	Hostname  string `json:"hostname" validate:"required"`
	IPAddress string `json:"ip_address" validate:"required"`
	OSInfo    string `json:"os_info"`
}

// AgentHeartbeat - запрос heartbeat от агента
type AgentHeartbeat struct {
	AgentID string `json:"agent_id" validate:"required"`
}
