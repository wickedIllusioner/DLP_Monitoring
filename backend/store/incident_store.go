package store

import (
	"DLP_Server/models"
	"context"
	"time"
)

// SaveEvent - сохранение события от агента
func (s *GormStore) SaveEvent(ctx context.Context, event *models.Event) (int64, error) {
	if event.DetectedAt.IsZero() {
		event.DetectedAt = time.Now()
	}

	result := s.db.WithContext(ctx).Create(event)
	if result.Error != nil {
		return 0, result.Error
	}

	return event.ID, nil
}

// GetEvents - получение всех событий
func (s *GormStore) GetEvents(ctx context.Context) ([]models.Event, error) {
	var events []models.Event

	result := s.db.WithContext(ctx).
		Preload("Agent").
		Order("created_at DESC").
		Find(&events)

	if result.Error != nil {
		return nil, result.Error
	}

	return events, nil
}

// CreateIncident - создание инцидента
func (s *GormStore) CreateIncident(ctx context.Context, incident *models.Incident) (int64, error) {
	result := s.db.WithContext(ctx).Create(incident)
	if result.Error != nil {
		return 0, result.Error
	}

	return incident.ID, nil
}

// GetIncidents - получение всех инцидентов
func (s *GormStore) GetIncidents(ctx context.Context) ([]models.Incident, error) {
	var incidents []models.Incident

	result := s.db.WithContext(ctx).
		Preload("Event").
		Preload("Event.Agent").
		Preload("Policy").
		Order("created_at DESC").
		Find(&incidents)

	if result.Error != nil {
		return nil, result.Error
	}

	return incidents, nil
}

// GetIncident - получение инцидента по ID
func (s *GormStore) GetIncident(ctx context.Context, id int64) (*models.Incident, error) {
	var incident models.Incident

	result := s.db.WithContext(ctx).
		Preload("Event").
		Preload("Event.Agent").
		Preload("Policy").
		Where("id = ?", id).
		First(&incident)

	if result.Error != nil {
		return nil, result.Error
	}

	return &incident, nil
}

// UpdateIncidentStatus - обновление статуса инцидента
func (s *GormStore) UpdateIncidentStatus(ctx context.Context, incident *models.Incident) error {
	updates := map[string]interface{}{
		"status":      incident.Status,
		"resolved_by": incident.ResolvedBy,
	}

	if incident.Status == "resolved" || incident.Status == "false_positive" {
		now := time.Now()
		updates["resolved_at"] = &now
	}

	result := s.db.WithContext(ctx).
		Model(&models.Incident{}).
		Where("id = ?", incident.ID).
		Updates(updates)

	if result.Error != nil {
		return result.Error
	}

	return nil
}

// GetIncidentStats - получение статистики по инцидентам
func (s *GormStore) GetIncidentStats(ctx context.Context) (*models.IncidentStats, error) {
	var stats models.IncidentStats

	totalResult := s.db.WithContext(ctx).
		Model(&models.Incident{}).
		Count(&stats.TotalIncidents)
	if totalResult.Error != nil {
		return nil, totalResult.Error
	}

	statusCounts := make(map[string]int64)
	var statusResults []struct {
		Status string
		Count  int64
	}

	statusQuery := s.db.WithContext(ctx).
		Model(&models.Incident{}).
		Select("status, COUNT(*) as count").
		Group("status").
		Find(&statusResults)
	if statusQuery.Error != nil {
		return nil, statusQuery.Error
	}

	for _, result := range statusResults {
		statusCounts[result.Status] = result.Count
	}

	stats.NewIncidents = statusCounts["new"]
	stats.Investigating = statusCounts["investigating"]
	stats.Resolved = statusCounts["resolved"]
	stats.FalsePositive = statusCounts["false_positive"]

	var severityResults []struct {
		Severity string
		Count    int64
	}

	severityQuery := s.db.WithContext(ctx).
		Model(&models.Incident{}).
		Select("severity, COUNT(*) as count").
		Group("severity").
		Order("severity").
		Find(&severityResults)
	if severityQuery.Error != nil {
		return nil, severityQuery.Error
	}

	stats.SeverityStats = make([]models.SeverityCount, len(severityResults))
	for i, result := range severityResults {
		stats.SeverityStats[i] = models.SeverityCount{
			Severity: result.Severity,
			Count:    result.Count,
		}
	}

	// Статистика по политикам
	var policyStats []models.PolicyCount

	// Получаем все политики с количеством инцидентов
	var policies []models.Policy
	if err := s.db.WithContext(ctx).Find(&policies).Error; err != nil {
		return nil, err
	}

	for _, policy := range policies {
		var count int64
		if err := s.db.WithContext(ctx).
			Model(&models.Incident{}).
			Where("policy_id = ?", policy.ID).
			Count(&count).Error; err != nil {
			return nil, err
		}

		if count > 0 {
			policyStats = append(policyStats, models.PolicyCount{
				PolicyID:   &policy.ID,
				PolicyName: policy.Name,
				Count:      count,
			})
		}
	}

	// Инциденты без политики
	var unknownCount int64
	if err := s.db.WithContext(ctx).
		Model(&models.Incident{}).
		Where("policy_id IS NULL").
		Count(&unknownCount).Error; err != nil {
		return nil, err
	}

	if unknownCount > 0 {
		policyStats = append(policyStats, models.PolicyCount{
			PolicyID:   nil,
			PolicyName: "Неизвестная политика",
			Count:      unknownCount,
		})
	}

	stats.PolicyStats = policyStats

	// Статистика по агентам
	var agentStats []models.AgentCount

	// Получаем всех агентов с событиями
	var agents []models.Agent
	if err := s.db.WithContext(ctx).Find(&agents).Error; err != nil {
		return nil, err
	}

	for _, agent := range agents {
		var count int64

		// Считаем инциденты через события агента
		if err := s.db.WithContext(ctx).
			Model(&models.Incident{}).
			Joins("JOIN events ON incidents.event_id = events.id").
			Where("events.agent_id = ?", agent.ID).
			Count(&count).Error; err != nil {
			return nil, err
		}

		if count > 0 {
			agentStats = append(agentStats, models.AgentCount{
				AgentID:   agent.ID,
				Hostname:  agent.Hostname,
				AgentUUID: agent.AgentUUID,
				Count:     count,
			})
		}
	}

	stats.AgentStats = agentStats

	// Последние 10 инцидентов
	var recentIncidents []models.Incident
	recentQuery := s.db.WithContext(ctx).
		Preload("Event").
		Preload("Event.Agent").
		Preload("Policy").
		Order("created_at DESC").
		Limit(10).
		Find(&recentIncidents)
	if recentQuery.Error != nil {
		return nil, recentQuery.Error
	}

	stats.RecentIncidents = recentIncidents

	return &stats, nil
}
