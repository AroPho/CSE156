CXX = clang++
CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -Og
LDFLAGS = -lpthread
COMMON_SOURCES = 
TARGET_SOURCES = myweb.cpp
TEST_SOURCES = httpserver.cpp
COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)
TARGET_OBJECTS = $(TARGET_SOURCES:.cpp=.o)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
EXECUTABLE = myweb
TEST_EXECUTABLE = httpserver

.PHONY: all target tests

all: target tests

target: $(EXECUTABLE)

tests: $(TEST_EXECUTABLE)

$(EXECUTABLE): $(TARGET_OBJECTS)
    $(CXX) $(LDFLAGS) $^ -o $@

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
    $(CXX) $(LDFLAGS) $^ -o $@

.cpp.o:
    $(CXX) $(CXXFLAGS) $< -o $@