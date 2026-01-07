package handlers

import (
	"DLP_Server/store"
	"github.com/rs/zerolog"
)

type Handler struct {
	store  store.GormStore
	logger zerolog.Logger
}

func NewHandler(store store.GormStore, logger zerolog.Logger) *Handler {
	return &Handler{
		store:  store,
		logger: logger,
	}
}
