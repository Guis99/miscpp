# Builds the WASM driver used by index.html. Needs Emscripten (em++ on PATH).
#   make        -> build/driver.js (wasm embedded, so index.html opens straight from disk)
#   make clean

EMXX     ?= em++
CXXFLAGS := -std=c++20 -O2 -I..
LDFLAGS  := -sMODULARIZE -sEXPORT_NAME=createDriver -sSINGLE_FILE -sENVIRONMENT=web

OUT := build/driver.js

$(OUT): driver.cpp ../predictors.h ../primitives.h ../collect.h
	@mkdir -p $(dir $@)
	$(EMXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf build
