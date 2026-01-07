package handlers

import (
	"DLP_Server/models"
	"encoding/json"
	"github.com/go-chi/chi/v5"
	"net/http"
	"strconv"
)

// GetIncidents - получение списка инцидентов
func (h *Handler) GetIncidents(w http.ResponseWriter, r *http.Request) {
	incidents, err := h.store.GetIncidents(r.Context())
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения инцидентов")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(incidents)
}

// GetIncident - получение инцидента по ID
func (h *Handler) GetIncident(w http.ResponseWriter, r *http.Request) {
	idStr := chi.URLParam(r, "id")
	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil {
		http.Error(w, "Неверный ID инцидента", http.StatusBadRequest)
		return
	}

	incident, err := h.store.GetIncident(r.Context(), id)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения инцидента")
		http.Error(w, "Инцидент не найден", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(incident)
}

// UpdateIncidentStatus - обновление статуса инцидента
func (h *Handler) UpdateIncidentStatus(w http.ResponseWriter, r *http.Request) {
	idStr := chi.URLParam(r, "id")
	id, err := strconv.ParseInt(idStr, 10, 64)
	if err != nil {
		http.Error(w, "Неверный ID инцидента", http.StatusBadRequest)
		return
	}

	var req models.IncidentUpdate
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	if req.Status == "" {
		http.Error(w, "Обязательное поле: status", http.StatusBadRequest)
		return
	}

	incident := models.Incident{
		ID:         id,
		Status:     req.Status,
		ResolvedBy: &req.ResolvedBy,
	}

	err = h.store.UpdateIncidentStatus(r.Context(), &incident)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка обновления инцидента")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Статус инцидента обновлен",
	})
}
