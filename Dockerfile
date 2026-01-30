FROM golang:1.24-alpine AS builder
RUN apk add --no-cache git
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 GOOS=linux go build -o dlp-server main.go

FROM alpine:latest
RUN apk --no-cache add ca-certificates tzdata
RUN addgroup -S appgroup && adduser -S appuser -G appgroup
WORKDIR /app
COPY --from=builder --chown=appuser:appgroup /app/dlp-server .
RUN mkdir -p /app/logs && chown -R appuser:appgroup /app/logs

USER appuser
EXPOSE ${SERVER_PORT:-8080}
CMD ["./dlp-server"]