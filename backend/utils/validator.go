package utils

import (
	"regexp"
	"strings"
)

// ValidateAgentRegistration - проверка структуры регистрации агента
func ValidateAgentRegistration(agentID, hostname, ipAddress string) []string {
	var errors []string

	if agentID == "" {
		errors = append(errors, "agent_id обязателен")
	} else if len(agentID) > 100 {
		errors = append(errors, "agent_id не должен превышать 100 символов")
	}

	if hostname == "" {
		errors = append(errors, "hostname обязателен")
	} else if len(hostname) > 255 {
		errors = append(errors, "hostname не должен превышать 255 символов")
	}

	if ipAddress != "" {
		ipPattern := `^(\d{1,3}\.){3}\d{1,3}$`
		if matched, _ := regexp.MatchString(ipPattern, ipAddress); !matched {
			errors = append(errors, "неверный формат IP адреса")
		}
	}

	return errors
}

// ValidatePolicyCreate - проверка структуры создания политики
func ValidatePolicyCreate(name, pattern, severity string) []string {
	var errors []string

	if name == "" {
		errors = append(errors, "название политики обязательно")
	} else if len(name) > 100 {
		errors = append(errors, "название политики не должно превышать 100 символов")
	}

	if pattern == "" {
		errors = append(errors, "шаблон (pattern) обязателен")
	}

	validSeverities := map[string]bool{
		"info":     true,
		"low":      true,
		"medium":   true,
		"high":     true,
		"critical": true,
	}

	if !validSeverities[severity] {
		errors = append(errors, "неверный уровень серьезности. Допустимые значения: info, low, medium, high, critical")
	}

	return errors
}

// ValidateEventCreate - проверка структуры создания события
func ValidateEventCreate(agentID, filePath, fileName, eventType, contentSample string) []string {
	var errors []string

	if agentID == "" {
		errors = append(errors, "agent_id обязателен")
	}

	if filePath == "" {
		errors = append(errors, "file_path обязателен")
	}

	if fileName == "" {
		errors = append(errors, "file_name обязателен")
	} else if len(fileName) > 255 {
		errors = append(errors, "file_name не должен превышать 255 символов")
	}

	validEventTypes := map[string]bool{
		"created":       true,
		"modified":      true,
		"deleted":       true,
		"renamed":       true,
		"accessed":      true,
		"content_match": true,
	}

	if !validEventTypes[eventType] {
		errors = append(errors, "неверный тип события")
	}

	if contentSample == "" {
		errors = append(errors, "content_sample обязателен")
	}

	return errors
}

// ValidateIncidentUpdate - проверка структуры обновления инцидента
func ValidateIncidentUpdate(status, resolvedBy string) []string {
	var errors []string

	validStatuses := map[string]bool{
		"new":            true,
		"investigating":  true,
		"resolved":       true,
		"false_positive": true,
	}

	if !validStatuses[status] {
		errors = append(errors, "неверный статус инцидента")
	}

	if status == "resolved" && resolvedBy == "" {
		errors = append(errors, "resolved_by обязателен при установке статуса 'resolved'")
	}

	return errors
}

// ValidateID - проверка корректности ID
func ValidateID(idStr string) (bool, string) {
	if idStr == "" {
		return false, "ID не может быть пустым"
	}

	for _, ch := range idStr {
		if ch < '0' || ch > '9' {
			return false, "ID должен содержать только цифры"
		}
	}

	return true, ""
}

// SanitizeInput - очистка от потенциально опасных символов
func SanitizeInput(input string) string {
	dangerousChars := []string{"<", ">", "'", "\"", ";", "--", "/*", "*/"}
	sanitized := input
	for _, char := range dangerousChars {
		sanitized = strings.ReplaceAll(sanitized, char, "")
	}
	sanitized = strings.TrimSpace(sanitized)

	return sanitized
}
