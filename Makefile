CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

BUILD   := build
CORE    := src/core/catalogue.c src/core/search.c
CLI     := src/main.c src/cli/cli.c

# On Windows without a POSIX shell, make runs recipes through cmd.exe.
ifeq ($(OS),Windows_NT)
    MKDIR  = if not exist $(BUILD) mkdir $(BUILD)
    RMDIR  = if exist $(BUILD) rmdir /s /q $(BUILD)
    PYTHON ?= python
else
    MKDIR  = mkdir -p $(BUILD)
    RMDIR  = rm -rf $(BUILD)
    PYTHON ?= python3
endif

.PHONY: all test check clean

all: $(BUILD)/myna

$(BUILD):
	$(MKDIR)

$(BUILD)/myna: $(CORE) $(CLI) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD)/test_search: tests/test_search.c $(CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

test: $(BUILD)/test_search
	./$(BUILD)/test_search data/concepts.csv

check:
	$(PYTHON) scripts/check_catalogue.py data/concepts.csv

clean:
	$(RMDIR)
