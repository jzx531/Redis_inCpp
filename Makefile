#
# make        : build executable
# make clean  : remove build and output files
#

CXX = g++

CXXFLAGS := -std=c++20 -Wall -Wextra -g -march=x86-64 -mtune=generic  -fcoroutines
LFLAGS =

ifeq ($(OS),Windows_NT)
	LFLAGS += -static-libstdc++ -static-libgcc
	MAIN := main.exe
	RM := del /q /f
	MD := mkdir
	FIXPATH = $(subst /,\,$1)
else
	MAIN := main
	RM := rm -f
	MD := mkdir -p
	FIXPATH = $1
endif

# =========================
# Directories
# =========================
SRC     := src
INCLUDE := include
LIB     := lib
BUILD   := build
OUTPUT  := output

# =========================
# Include / Lib flags
# =========================
INCLUDES := -I$(INCLUDE)
LIBS := -L$(LIB) -lpthread 

# =========================
# Sources / Objects
# =========================
SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

TARGET := $(call FIXPATH,$(OUTPUT)/$(MAIN))

# =========================
# Rules
# =========================
all: $(BUILD) $(OUTPUT) $(TARGET)
	@echo Build complete

$(BUILD):
	$(MD) $(BUILD)

$(OUTPUT):
	$(MD) $(OUTPUT)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LFLAGS) $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -c $< -o $@

-include $(DEPS)

.PHONY: clean run

clean:
	$(RM) $(call FIXPATH,$(BUILD)/*.o)
	$(RM) $(call FIXPATH,$(BUILD)/*.d)
	$(RM) $(call FIXPATH,$(TARGET))
	@echo Clean complete

run: all
	./$(TARGET)