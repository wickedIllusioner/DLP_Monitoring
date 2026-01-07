package store

import (
	"DLP_Server/models"
	"context"
)

// GetPolicies - получение всех политик
func (s *GormStore) GetPolicies(ctx context.Context) ([]models.Policy, error) {
	var policies []models.Policy

	result := s.db.WithContext(ctx).
		Order("created_at DESC").
		Find(&policies)

	if result.Error != nil {
		return nil, result.Error
	}

	return policies, nil
}

// GetActivePolicies - получение активных политик
func (s *GormStore) GetActivePolicies(ctx context.Context) ([]models.Policy, error) {
	var policies []models.Policy

	result := s.db.WithContext(ctx).
		Where("is_active = ?", true).
		Order("created_at DESC").
		Find(&policies)

	if result.Error != nil {
		return nil, result.Error
	}

	return policies, nil
}

// CreatePolicy - создание новой политики
func (s *GormStore) CreatePolicy(ctx context.Context, policy *models.Policy) (int64, error) {
	result := s.db.WithContext(ctx).Create(policy)
	if result.Error != nil {
		return 0, result.Error
	}

	return policy.ID, nil
}

// UpdatePolicy - обновление политики
func (s *GormStore) UpdatePolicy(ctx context.Context, policy *models.Policy) error {
	result := s.db.WithContext(ctx).
		Model(&models.Policy{}).
		Where("id = ?", policy.ID).
		Updates(policy)

	if result.Error != nil {
		return result.Error
	}

	return nil
}

// DeletePolicy - удаление политики
func (s *GormStore) DeletePolicy(ctx context.Context, id int64) error {
	result := s.db.WithContext(ctx).
		Where("id = ?", id).
		Delete(&models.Policy{})

	if result.Error != nil {
		return result.Error
	}

	return nil
}

// GetPolicy - получение политики по ID
func (s *GormStore) GetPolicy(ctx context.Context, id int64) (*models.Policy, error) {
	var policy models.Policy

	result := s.db.WithContext(ctx).
		Where("id = ?", id).
		First(&policy)

	if result.Error != nil {
		return nil, result.Error
	}

	return &policy, nil
}
