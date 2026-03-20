BUILD_DIR := build
TARGET := $(BUILD_DIR)/Hello3D

.PHONY: all configure build run clean rebuild help

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build

help:
	@echo "Targets disponiveis:"
	@echo "  make          -> configura e compila"
	@echo "  make run      -> configura, compila e executa"
	@echo "  make clean    -> remove a pasta build"
	@echo "  make rebuild  -> clean + build"