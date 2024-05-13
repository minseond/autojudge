CC=gcc
CFLAGS=-Wall -g
EXEC=autojudge
SRC=autojudge.c

# Default target
all: $(EXEC) example1 example2

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

# Run example 1
example1: $(EXEC)
	@echo "Running Example 1..."
	@./$(EXEC) -i example1/input -a example1/answer -t 10 example1/pathname.c
	@echo "Example 1 completed."

# Run example 2
example2: $(EXEC)
	@echo "Running Example 2..."
	@./$(EXEC) -i example2/in -a example2/ans -t 10 example2/substr.c
	@echo "Example 2 completed."

# Clean the built files
clean:
	rm -f $(EXEC)

# Declare phony targets
.PHONY: all example1 example2 clean

