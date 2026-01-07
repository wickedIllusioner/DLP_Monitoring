package store

import (
	"DLP_Server/models"
	"context"
)

// Store - интерфейс для работы с данными
type Store interface {
	// Агенты
	CreateAgent(ctx context.Context, agent *models.Agent) (int64, error)
	GetAgents(ctx context.Context) ([]models.Agent, error)
	GetAgent(ctx context.Context, id int64) (*models.Agent, error)
	GetAgentByUUID(ctx context.Context, agentID string) (*models.Agent, error)
	UpdateAgentHeartbeat(ctx context.Context, agentID string) error
	DeleteAgent(ctx context.Context, id int64) error
	DeleteAgentByUUID(ctx context.Context, agentID string) error

	// Политики
	GetPolicies(ctx context.Context) ([]models.Policy, error)
	GetPolicy(ctx context.Context, id int64) (*models.Policy, error)
	GetActivePolicies(ctx context.Context) ([]models.Policy, error)
	CreatePolicy(ctx context.Context, policy *models.Policy) (int64, error)
	UpdatePolicy(ctx context.Context, policy *models.Policy) error
	DeletePolicy(ctx context.Context, id int64) error

	// События
	SaveEvent(ctx context.Context, event *models.Event) (int64, error)
	GetEvents(ctx context.Context) ([]models.Event, error)

	// Инциденты
	CreateIncident(ctx context.Context, incident *models.Incident) (int64, error)
	GetIncidents(ctx context.Context) ([]models.Incident, error)
	GetIncident(ctx context.Context, id int64) (*models.Incident, error)
	UpdateIncidentStatus(ctx context.Context, incident *models.Incident) error

	Close() error
}
