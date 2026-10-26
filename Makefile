CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

BUILD   := build
CORE    := src/core/catalogue.c src/core/search.c src/core/prompt.c
CLI     := src/main.c src/cli/cli.c src/cli/terminal.c src/cli/logo.c \
           src/cli/json.c src/cli/mcp.c src/cli/report.c
HEADERS := $(wildcard include/myna/*.h src/cli/*.h)

# On Windows without a POSIX shell, make runs recipes through cmd.exe.
ifeq ($(OS),Windows_NT)
    MKDIR  = if not exist $(BUILD) mkdir $(BUILD)
    RMDIR  = if exist $(BUILD) rmdir /s /q $(BUILD)
    PYTHON ?= python
    WINDRES ?= windres
    RES    := $(BUILD)/myna_res.o
else
    MKDIR  = mkdir -p $(BUILD)
    RMDIR  = rm -rf $(BUILD)
    PYTHON ?= python3
    RES    :=
endif

.PHONY: all test check coverage clean

all: $(BUILD)/myna

$(BUILD):
	$(MKDIR)

$(BUILD)/myna: $(CORE) $(CLI) $(HEADERS) $(RES) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) $(CLI) $(RES) -o $@

# The Windows icon, embedded in myna.exe.
$(BUILD)/myna_res.o: assets/myna.rc assets/myna.ico | $(BUILD)
	$(WINDRES) --include-dir=assets assets/myna.rc -O coff -o $@

$(BUILD)/test_search: tests/test_search.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_search.c $(CORE) -o $@

$(BUILD)/test_prompt: tests/test_prompt.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_prompt.c $(CORE) -o $@

$(BUILD)/test_mcp: tests/test_mcp.c src/cli/json.c src/cli/mcp.c src/cli/report.c $(CORE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) -Isrc/cli $(CFLAGS) tests/test_mcp.c src/cli/json.c src/cli/mcp.c src/cli/report.c $(CORE) -o $@

test: $(BUILD)/test_search $(BUILD)/test_prompt $(BUILD)/test_mcp
	./$(BUILD)/test_search data/concepts.csv
	./$(BUILD)/test_prompt
	./$(BUILD)/test_mcp data/concepts.csv

check:
	$(PYTHON) scripts/check_catalogue.py data/concepts.csv

# Report how well the catalogue answers the requests in tests/requests.csv.
coverage: $(BUILD)/myna
	$(PYTHON) scripts/coverage.py

clean:
	$(RMDIR)
