SRC_DIR     := src
INCLUDE_DIR := includes
BUILD_DIR   := build

SRC_FILES   := $(wildcard $(SRC_DIR)/*.cpp)
TARGET      := inquisitor

DOCKER_COMPOSE_FILE := docker-compose.yaml

CXX      := g++
CXXFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -std=c++17 -pthread
LDFLAGS  := -lpcap

.PHONY: all build docker clean stop

# Default: build the Docker environment
all: docker

# Build the binary (runs inside the container via Dockerfile, or locally on Linux)
build:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_FILES) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)
	@echo "Build completed successfully!"

# Bring the full test environment up
docker:
	docker compose -f $(DOCKER_COMPOSE_FILE) up --build

# Tear down containers and remove the build directory
clean:
	@rm -rf $(BUILD_DIR)
	docker compose -f $(DOCKER_COMPOSE_FILE) down --remove-orphans
	@echo "Cleaned up."

# Stop containers without removing build artifacts
stop:
	docker compose -f $(DOCKER_COMPOSE_FILE) down --remove-orphans
	@echo "Docker containers stopped."
