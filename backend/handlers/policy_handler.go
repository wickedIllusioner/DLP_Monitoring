package handlers

import (
	"DLP_Server/models"
	"encoding/json"
	"github.com/go-chi/chi/v5"
	"net/http"
	"strconv"
)

// GetPolicies - получение списка политик
func (h *Handler) GetPolicies(w http.ResponseWriter, r *http.Request) {
	policies, err := h.store.GetPolicies(r.Context())
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения политик")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(policies)
}

// GetPoliciesForAgent - получение политик в формате для агента
func (h *Handler) GetPoliciesForAgent(w http.ResponseWriter, r *http.Request) {
	policies, err := h.store.GetActivePolicies(r.Context())
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения политик для агента")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	type AgentPolicy struct {
		ID       int64  `json:"id"`
		Name     string `json:"name"`
		Pattern  string `json:"pattern"`
		Severity string `json:"severity"`
	}

	agentPolicies := make([]AgentPolicy, len(policies))
	for i, p := range policies {
		agentPolicies[i] = AgentPolicy{
			ID:       p.ID,
			Name:     p.Name,
			Pattern:  p.Pattern,
			Severity: p.Severity,
		}
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(agentPolicies)
}

// CreatePolicy - создание новой политики
func (h *Handler) CreatePolicy(w http.ResponseWriter, r *http.Request) {
	var req models.PolicyCreate
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	if req.Name == "" || req.Pattern == "" {
		http.Error(w, "Обязательные поля: name, pattern", http.StatusBadRequest)
		return
	}

	policy := models.Policy{
		Name:        req.Name,
		Description: req.Description,
		Pattern:     req.Pattern,
		Severity:    req.Severity,
		IsActive:    req.IsActive,
	}

	id, err := h.store.CreatePolicy(r.Context(), &policy)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка создания политики")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"id":      id,
		"message": "Политика создана",
	})
}

// UpdatePolicy - обновление политики
func (h *Handler) UpdatePolicy(w http.ResponseWriter, r *http.Request) {
	idStr := chi.URLParam(r, "id")
	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil {
		http.Error(w, "Неверный ID политики", http.StatusBadRequest)
		return
	}

	var req models.PolicyUpdate
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	policy := models.Policy{
		ID:          id,
		Name:        req.Name,
		Description: req.Description,
		Pattern:     req.Pattern,
		Severity:    req.Severity,
	}

	if req.IsActive != nil {
		policy.IsActive = *req.IsActive
	}

	err = h.store.UpdatePolicy(r.Context(), &policy)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка обновления политики")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Политика обновлена",
	})
}

// DeletePolicy - удаление политики
func (h *Handler) DeletePolicy(w http.ResponseWriter, r *http.Request) {
	idStr := chi.URLParam(r, "id")
	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil {
		http.Error(w, "Неверный ID политики", http.StatusBadRequest)
		return
	}

	err = h.store.DeletePolicy(r.Context(), id)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка удаления политики")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Политика удалена",
	})
}
