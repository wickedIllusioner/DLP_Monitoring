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
