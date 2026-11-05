# Build tất cả project. Dùng: make, make run-01, make test-02, make tsan-02, make run-03 ...
CXX      ?= c++
ARCH     := $(shell uname -m)
ifeq ($(ARCH),arm64)
  CPUFLAG := -mcpu=native
else
  CPUFLAG := -march=native
endif
CXXFLAGS := -std=c++20 -O2 $(CPUFLAG) -g -Wall -Wextra -pthread
BUILD    := build

TARGETS := $(BUILD)/01-hardware-effects \
           $(BUILD)/02-spsc-test $(BUILD)/02-spsc-bench \
           $(BUILD)/03-book-test $(BUILD)/03-book-bench

all: $(TARGETS)

$(BUILD):
	mkdir -p $(BUILD)

COMMON := projects/common/bench.hpp

$(BUILD)/01-hardware-effects: projects/01-hardware-effects/main.cpp $(COMMON) | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/02-spsc-%: projects/02-spsc-queue/%.cpp projects/02-spsc-queue/*.hpp $(COMMON) | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/03-book-%: projects/03-order-book/%.cpp projects/03-order-book/*.hpp $(COMMON) | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

run-01: $(BUILD)/01-hardware-effects
	./$<
test-02: $(BUILD)/02-spsc-test
	./$<
run-02: $(BUILD)/02-spsc-bench
	./$<
tsan-02: | $(BUILD)
	$(CXX) $(CXXFLAGS) -O1 -fsanitize=thread projects/02-spsc-queue/test.cpp -o $(BUILD)/02-spsc-test-tsan
	./$(BUILD)/02-spsc-test-tsan
test-03: $(BUILD)/03-book-test
	./$<
run-03: $(BUILD)/03-book-bench
	./$<

clean:
	rm -rf $(BUILD)

.PHONY: all clean run-01 test-02 run-02 tsan-02 test-03 run-03
