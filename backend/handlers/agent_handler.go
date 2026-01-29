package handlers

import (
	"DLP_Server/models"
	"encoding/json"
	"github.com/go-chi/chi/v5"
	"net/http"
	"strconv"
)

// RegisterAgent - регистрация нового агента
func (h *Handler) RegisterAgent(w http.ResponseWriter, r *http.Request) {
	var req models.AgentRegistration
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		h.logger.Error().Err(err).Msg("Ошибка декодирования запроса")
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	if req.AgentID == "" || req.Hostname == "" {
		http.Error(w, "Обязательные поля: agent_id, hostname", http.StatusBadRequest)
		return
	}

	agent := models.Agent{
		AgentUUID: req.AgentID,
		Hostname:  req.Hostname,
		IPAddress: req.IPAddress,
		OSInfo:    req.OSInfo,
	}

	id, err := h.store.CreateAgent(r.Context(), &agent)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка создания агента")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"id":      id,
		"message": "Агент зарегистрирован",
	})
}

// GetAgents - получение списка агентов
func (h *Handler) GetAgents(w http.ResponseWriter, r *http.Request) {
	agents, err := h.store.GetAgents(r.Context())
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения агентов")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(agents)
}

// UpdateAgentHeartbeat - обновление heartbeat агента
func (h *Handler) UpdateAgentHeartbeat(w http.ResponseWriter, r *http.Request) {
	agentUUID := chi.URLParam(r, "id")

	if agentUUID == "" {
		http.Error(w, "UUID агента обязателен", http.StatusBadRequest)
		return
	}

	err := h.store.UpdateAgentHeartbeat(r.Context(), agentUUID)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка обновления heartbeat")
		http.Error(w, "Агент не найден", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Heartbeat обновлен",
	})
}

// DeleteAgent - удаление агента
func (h *Handler) DeleteAgent(w http.ResponseWriter, r *http.Request) {
	idStr := chi.URLParam(r, "id")
	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil {
		http.Error(w, "Неверный ID агента", http.StatusBadRequest)
		return
	}

	err = h.store.DeleteAgent(r.Context(), id)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка удаления агента")
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Агент удален",
	})
}
