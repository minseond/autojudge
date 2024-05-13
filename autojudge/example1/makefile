# Compiler and Compiler Flags
CC=gcc
CFLAGS=-Wall -g

# Executables
EXEC1=autojudge
EXEC2=pathname

# Source files
SRC1=autojudge.c
SRC2=pathname.c

# Default target
all: $(EXEC1) $(EXEC2) run

# Compile autojudge
$(EXEC1): $(SRC1)
	$(CC) $(CFLAGS) -o $(EXEC1) $(SRC1)
	echo "Compiled $(EXEC1) successfully."

# Compile pathname
$(EXEC2): $(SRC2)
	$(CC) $(CFLAGS) -o $(EXEC2) $(SRC2)
	echo "Compiled $(EXEC2) successfully."

# Run executables
run: $(EXEC1) $(EXEC2)
	@echo "Running $(EXEC1)..."
	@./$(EXEC1)
	@echo "Running $(EXEC2)..."
	@./$(EXEC2)

# Compare outputs
compare: run
	@echo "Comparing outputs of $(EXEC1) and $(EXEC2)..."
	@./$(EXEC1) > output1.txt
	@./$(EXEC2) > output2.txt
	@diff output1.txt output2.txt && echo "No differences found." || echo "Differences found!"

# Clean the built files and outputs
clean:
	rm -f $(EXEC1) $(EXEC2) output1.txt output2.txt
	echo "Cleaned up build files and outputs."

# Declare phony targets
.PHONY: all run compare clean

# Run executables
run: $(EXEC1) $(EXEC2)
	@echo "Running $(EXEC1)..."
	@./$(EXEC1)
	@echo "Running $(EXEC2)..."
	@./$(EXEC2)

