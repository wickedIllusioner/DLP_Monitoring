package handlers

import (
	"DLP_Server/models"
	"encoding/json"
	"net/http"
)

// HandleEvent - обработка события от агента
func (h *Handler) HandleEvent(w http.ResponseWriter, r *http.Request) {
	var req models.EventCreate
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		h.logger.Error().Err(err).Msg("Ошибка декодирования события")
		http.Error(w, "Неверный формат JSON", http.StatusBadRequest)
		return
	}

	if req.AgentID == "" || req.FilePath == "" || req.EventType == "" {
		http.Error(w, "Обязательные поля: agent_id, file_path, event_type", http.StatusBadRequest)
		return
	}

	agent, err := h.store.GetAgentByUUID(r.Context(), req.AgentID)
	if err != nil {
		h.logger.Error().Err(err).Msg("Агент не найден")
		http.Error(w, "Агент не найден", http.StatusNotFound)
		return
	}

	event := models.Event{
		AgentID:       agent.ID,
		FilePath:      req.FilePath,
		FileName:      req.FileName,
		EventType:     req.EventType,
		FileSize:      req.FileSize,
		ContentSample: req.ContentSample,
		DetectedAt:    req.DetectedAt,
	}

	eventID, err := h.store.SaveEvent(r.Context(), &event)
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка сохранения события")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	// Обработка обнаруженного нарушения
	if req.IsViolation {
		var policyID *int64 = nil
		var severity string = "medium"

		if req.MatchedPolicy != nil {
			policies, err := h.store.GetPolicies(r.Context())
			if err == nil {
				for _, policy := range policies {
					if policy.Name == *req.MatchedPolicy {
						policyID = &policy.ID
						severity = policy.Severity
						break
					}
				}
			}
		}
		if req.Severity != nil {
			severity = *req.Severity
		}

		incident := models.Incident{
			EventID:        eventID,
			PolicyID:       policyID,
			Severity:       severity,
			Status:         "new",
			MatchedContent: req.ContentSample,
		}

		if req.ViolationType != nil {
			incident.MatchedContent = *req.ViolationType + ": " + incident.MatchedContent
		}

		_, err := h.store.CreateIncident(r.Context(), &incident)
		if err != nil {
			h.logger.Error().Msg("Ошибка создания инцидента")
		}
		h.logger.Info().
			Str("agent_id", req.AgentID).
			Str("file_path", req.FilePath).
			Bool("violation", true).
			Msg("Создан инцидент по нарушению")
	} else {
		h.logger.Debug().
			Str("agent_id", req.AgentID).
			Str("file_path", req.FilePath).
			Msg("Событие сохранено")
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"event_id":            eventID,
		"status":              "Обработано",
		"violation_processed": req.IsViolation,
	})
}

// GetEvents - получение списка событий
func (h *Handler) GetEvents(w http.ResponseWriter, r *http.Request) {
	events, err := h.store.GetEvents(r.Context())
	if err != nil {
		h.logger.Error().Err(err).Msg("Ошибка получения событий")
		http.Error(w, "Ошибка сервера", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(events)
}
