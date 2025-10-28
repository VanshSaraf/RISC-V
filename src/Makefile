# Compiler to use
CC = g++

# Source files
SOURCES = forwarding.cpp noforwarding.cpp

# Executable names
FORWARD_EXE = forward
NOFORWARD_EXE = noforward

# Output directory and file
OUTPUT_DIR = ../outputfiles
OUTPUT_FILE = $(OUTPUT_DIR)/output.txt

# CSV file
CSV_FILE = $(OUTPUT_DIR)/try.csv

# Default target
all: $(FORWARD_EXE) $(NOFORWARD_EXE)

# Compile forwarding.cpp into forward.exe
$(FORWARD_EXE): forwarding.cpp
	$(CC) -o $(FORWARD_EXE) forwarding.cpp

# Compile noforwarding.cpp into noforward.exe
$(NOFORWARD_EXE): noforwarding.cpp
	$(CC) -o $(NOFORWARD_EXE) noforwarding.cpp

# Clean executables
clean:
	rm -f $(FORWARD_EXE) $(NOFORWARD_EXE)

# Copy output.txt to try.csv in the same folder
csv:
	cp $(OUTPUT_FILE) $(CSV_FILE)

# Prevent make from treating these as file targets
.PHONY: all forward noforward clean csv

# Handle extra arguments to forward/noforward targets
%:
	@: