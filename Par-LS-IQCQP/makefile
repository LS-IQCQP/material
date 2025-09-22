CXX = g++
CXXFLAGS = -static -O3 -lm -pthread -std=c++17 -Ideps/fmt/include -Ideps/gsl/build -Ldeps/fmt/build -Ldeps/gsl/build -lfmt -lgsl -lgslcblas
DEBUGFLAGS = -g -DDEBUG
SRC = $(wildcard *.cpp)
BUILD_DIR = build

# Ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

ifeq ($(DEBUG),1)
override CXXFLAGS += $(DEBUGFLAGS)
override CXXFLAGS := $(filter-out -O3, $(CXXFLAGS))
endif

# Handle any command line variables set to 1
CMDLINE_VARS := $(filter-out $(MAKECMDGOALS), $(foreach v, $(.VARIABLES), \
                   $(if $(filter command line, $(origin $(v))), $(v))))
EXCLUDED_VARS = DEBUG CXX CXXFLAGS SRC BUILD_DIR MAKEFLAGS MAKELEVEL MAKE
FILTERED_VARS := $(filter-out $(EXCLUDED_VARS), $(CMDLINE_VARS))
$(foreach var, $(FILTERED_VARS), \
    $(if $(filter 1, $($(var))), \
        $(eval override CXXFLAGS += -D$(var)) \
    ) \
)

Qseek: $(SRC) | $(BUILD_DIR)
	$(CXX) $(filter-out propagation_test.cpp checker.cpp, $(SRC)) $(CXXFLAGS) -o $(BUILD_DIR)/Qseek

test: $(SRC) | $(BUILD_DIR)
	$(CXX) $(filter-out call.cpp checker.cpp, $(SRC)) $(CXXFLAGS) -o $(BUILD_DIR)/test

checker: $(SRC) | $(BUILD_DIR)
	$(CXX) $(filter-out propagation_test.cpp call.cpp, $(SRC)) $(CXXFLAGS) -o $(BUILD_DIR)/checker

.PHONY: all clean

all: Qseek test checker

clean:
	rm -rf $(BUILD_DIR)