V               ?= @

.DEFAULT_GOAL := help

BUILD_DIR := build
BUILD_DIR_32 := $(BUILD_DIR)/build32
BUILD_DIR_64 := $(BUILD_DIR)/build64
BUILD_DIR_DOCKER := $(BUILD_DIR)/docker
BUILD_DIR_ZIP := $(BUILD_DIR)/zip

DOC_DIR := doc

DIST_DIR := dist

DOCKER_CONTAINER_NAME := "segatools-build"
DOCKER_IMAGE_NAME     := "segatools:build"

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

include Package.mk

.PHONY: build # Build the project
build:
	$(V)meson --cross cross-mingw-32.txt $(BUILD_DIR_32)
	$(V)ninja -C $(BUILD_DIR_32)
	$(V)meson --cross cross-mingw-64.txt $(BUILD_DIR_64)
	$(V)ninja -C $(BUILD_DIR_64)

.PHONY: dist # Build and create a zip distribution package
dist: build zip

.PHONY: zip # Create a zip distribution pacakge
zip: $(BUILD_DIR_ZIP)/segatools.zip

.PHONY: clean # Cleanup build output
clean:
	$(V)rm -rf $(BUILD_DIR) subprojects/capnhook

.PHONY: build-docker # Build the project in a docker container
build-docker:
	$(V)docker rm -f $(DOCKER_CONTAINER_NAME) 2> /dev/null || true
	$(V)docker build -t $(DOCKER_IMAGE_NAME) -f Dockerfile .
	$(V)docker create --name $(DOCKER_CONTAINER_NAME) $(DOCKER_IMAGE_NAME)
	$(V)rm -rf $(BUILD_DIR_DOCKER)
	$(V)mkdir -p $(BUILD_DIR_DOCKER)
	$(V)docker cp $(DOCKER_CONTAINER_NAME):/segatools/$(BUILD_DIR_ZIP) $(BUILD_DIR_DOCKER)

# -----------------------------------------------------------------------------
# Utility, combo and alias targets
# -----------------------------------------------------------------------------

# Help screen note:
# Variables that need to be displayed in the help screen need to strictly
# follow the pattern "^[A-Z_]+ \?= .* # .*".
# Targets that need to be displayed in the help screen need to add a separate
# phony definition strictly following the pattern "^\.PHONY\: .* # .*".

.PHONY: help # Print help screen
help:
	$(V)echo segatools makefile.
	$(V)echo
	$(V)echo "Environment variables:"
	$(V)grep -E '^[A-Z_]+ \?\= .* #' Makefile | gawk 'match($$0, /([A-Z_]+) \?= [$$\(]*([^\)]*)[\)]{0,1} # (.*)/, a) { printf("  \033[0;35m%-25s \033[0;0m%-45s [%s]\n", a[1], a[3], a[2]) }'
	$(V)echo ""
	$(V)echo "Targets:"
	$(V)grep '^.PHONY: .* #' Makefile | gawk 'match($$0, /\.PHONY: (.*) # (.*)/, a) { printf("  \033[0;32m%-25s \033[0;0m%s\n", a[1], a[2]) }'
