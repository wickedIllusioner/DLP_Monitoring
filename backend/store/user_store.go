package store

import (
	"DLP_Server/models"
	"context"
)

// GetUserByUsername - получение пользователя по имени
func (s *GormStore) GetUserByUsername(ctx context.Context, username string) (*models.User, error) {
	var user models.User

	result := s.db.WithContext(ctx).
		Where("username = ?", username).
		First(&user)

	if result.Error != nil {
		return nil, result.Error
	}
	return &user, nil
}
