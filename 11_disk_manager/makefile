# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra -lm

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# All tests files
all : pingpong-mqueue

# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Compilation rule for test files
pingpong-mqueue: ppos_core.o pingpong-mqueue.o queue.o
	gcc -o $@ $^ $(CFLAGS)

# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Declaring clean function
clean :
	$(RM) *.o *.log *.txt
# Declaring purge clean function
purge : clean
	$(RM) pingpong-mqueue