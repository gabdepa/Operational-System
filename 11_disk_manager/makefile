# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra -lm -lrt

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# All tests files
all : pingpong-disco1 pingpong-disco2

# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Compilation rule for test files
pingpong-disco1: ppos_core.o ppos_disk.o disk.o pingpong-disco1.o queue.o
	gcc -o $@ $^ $(CFLAGS)

# Compilation rule for test files
pingpong-disco2: ppos_core.o ppos_disk.o disk.o pingpong-disco2.o queue.o
	gcc -o $@ $^ $(CFLAGS)

# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Declaring clean function
clean :
	$(RM) *.o *.log *.txt
# Declaring purge clean function
purge : clean
	$(RM) pingpong-disco1 pingpong-disco2