package models

import (
	"golang.org/x/crypto/bcrypt"
)

type User struct {
	ID           int64  `json:"id" gorm:"primaryKey;autoIncrement"`
	Username     string `json:"username" gorm:"size:50;uniqueIndex;not null"`
	PasswordHash string `json:"-" gorm:"size:255;not null"`
	IsActive     bool   `json:"is_active" gorm:"default:true"`
}

type UserLogin struct {
	Username string `json:"username" validate:"required"`
	Password string `json:"password" validate:"required"`
}

type UserResponse struct {
	ID       int64  `json:"id"`
	Username string `json:"username"`
	IsActive bool   `json:"is_active"`
}

// CheckPassword - проверка пароля
func (u *User) CheckPassword(password string) bool {
	err := bcrypt.CompareHashAndPassword([]byte(u.PasswordHash), []byte(password))
	return err == nil
}

// SetPassword - установка нового пароля
func (u *User) SetPassword(password string) error {
	hash, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	if err != nil {
		return err
	}
	u.PasswordHash = string(hash)
	return nil
}
