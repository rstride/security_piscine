# Define project directories
SRC_DIR := src
INCLUDE_DIR := includes
BUILD_DIR := build

# Define source files and target binary
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
TARGET := inquisitor

# Docker-related definitions
DOCKER_COMPOSE_FILE := docker-compose.yml

# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -std=c++17
LDFLAGS := -lpcap

# Default rule
all: build docker

# Rule to build the C++ project
build:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_FILES) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)
	@echo "Build completed successfully!"

# Rule to run Docker Compose
docker:
	@docker-compose -f $(DOCKER_COMPOSE_FILE) up --build
	@echo "Docker Compose up and running!"

# Rule to clean the build directory and Docker containers
clean:
	@rm -rf $(BUILD_DIR)
	@docker-compose -f $(DOCKER_COMPOSE_FILE) down
	@echo "Cleaned up build directory and Docker containers."

# Rule to stop Docker containers
stop:
	@docker-compose -f $(DOCKER_COMPOSE_FILE) down
	@echo "Docker containers stopped."

.PHONY: all build docker clean stop