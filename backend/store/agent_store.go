package store

import (
	"DLP_Server/models"
	"context"
	"fmt"
	"time"
)

// CreateAgent - создание нового агента
func (s *GormStore) CreateAgent(ctx context.Context, agent *models.Agent) (int64, error) {
	now := time.Now()
	agent.LastSeen = now

	result := s.db.WithContext(ctx).Create(agent)
	if result.Error != nil {
		return 0, result.Error
	}

	return agent.ID, nil
}

// GetAgents - получение всех агентов
func (s *GormStore) GetAgents(ctx context.Context) ([]models.Agent, error) {
	var agents []models.Agent

	result := s.db.WithContext(ctx).
		Order("created_at DESC").
		Find(&agents)

	if result.Error != nil {
		return nil, result.Error
	}

	return agents, nil
}

// GetAgentByUUID - получение агента по UUID (agent_uuid)
func (s *GormStore) GetAgentByUUID(ctx context.Context, agentUUID string) (*models.Agent, error) {
	var agent models.Agent

	result := s.db.WithContext(ctx).
		Where("agent_uuid = ?", agentUUID).
		First(&agent)

	if result.Error != nil {
		return nil, result.Error
	}

	return &agent, nil
}

// UpdateAgentHeartbeat - обновление времени последней активности агента
func (s *GormStore) UpdateAgentHeartbeat(ctx context.Context, agentUUID string) error {
	result := s.db.WithContext(ctx).
		Model(&models.Agent{}).
		Where("agent_id = ?", agentUUID).
		Updates(map[string]interface{}{
			"last_seen":  time.Now(),
			"updated_at": time.Now(),
		})

	if result.Error != nil {
		return result.Error
	}

	if result.RowsAffected == 0 {
		return fmt.Errorf("агент не найден")
	}

	return nil
}

// DeleteAgent - удаление агента
func (s *GormStore) DeleteAgent(ctx context.Context, id int64) error {
	result := s.db.WithContext(ctx).
		Where("id = ?", id).
		Delete(&models.Agent{})

	if result.Error != nil {
		return result.Error
	}

	if result.RowsAffected == 0 {
		return fmt.Errorf("агент с ID %d не найден", id)
	}

	return nil
}

// DeleteAgentByUUID - удаление агента по UUID
func (s *GormStore) DeleteAgentByUUID(ctx context.Context, agentUUID string) error {
	result := s.db.WithContext(ctx).
		Where("agent_id = ?", agentUUID).
		Delete(&models.Agent{})

	if result.Error != nil {
		return result.Error
	}

	if result.RowsAffected == 0 {
		return fmt.Errorf("агент с UUID %s не найден", agentUUID)
	}

	return nil
}
