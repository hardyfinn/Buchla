# Path to the Rack SDK (override on the command line: make RACK_DIR=...)
RACK_DIR ?= ../Rack-SDK

FLAGS +=
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res/Buchla259t.svg
DISTRIBUTABLES += $(wildcard LICENSE*)

ifneq ($(MAKECMDGOALS),test)
include $(RACK_DIR)/plugin.mk
endif

# Engine checks, no Rack SDK needed (test/render demo.wav also renders the demo)
test:
	$(CXX) -O2 -Wall -Wextra -std=c++17 -Isrc test/render.cpp -o test/render && test/render
.PHONY: test
