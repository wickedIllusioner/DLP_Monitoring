package handlers

import (
	"DLP_Server/models"
	"encoding/json"
	"net/http"
)

// Login - авторизация пользователя
func (h *Handler) Login(w http.ResponseWriter, r *http.Request) {
	var req models.UserLogin
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		h.logger.Error().Err(err).Msg("Ошибка декодирования запроса")
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	if req.Username == "" || req.Password == "" {
		http.Error(w, "Обязательные поля: username, password", http.StatusBadRequest)
		return
	}

	user, err := h.store.GetUserByUsername(r.Context(), req.Username)
	if err != nil {
		h.logger.Warn().Str("username", req.Username).Msg("Пользователь не найден")
		http.Error(w, "Неверные учетные данные", http.StatusUnauthorized)
		return
	}

	if !user.IsActive {
		h.logger.Warn().Str("username", req.Username).Msg("Пользователь неактивен")
		http.Error(w, "Учетная запись неактивна", http.StatusForbidden)
		return
	}

	if !user.CheckPassword(req.Password) {
		h.logger.Warn().Str("username", req.Username).Msg("Неверный пароль")
		http.Error(w, "Неверные учетные данные", http.StatusUnauthorized)
		return
	}

	response := models.UserResponse{
		ID:       user.ID,
		Username: user.Username,
		IsActive: user.IsActive,
	}

	h.logger.Info().
		Str("username", user.Username).
		Msg("Успешная авторизация")

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
		"user":    response,
	})
}
