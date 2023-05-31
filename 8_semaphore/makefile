# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# All tests files
all : pingpong-semaphore pingpong-racecond
# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Compilation rule for test files
pingpong-semaphore: ppos_core.o pingpong-semaphore.o queue.o
	gcc -o $@ $^

pingpong-racecond: ppos_core.o pingpong-racecond.o queue.o
	gcc -o $@ $^

# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Declaring clean function
clean :
	$(RM) *.o *.log *.txt
# Declaring purge clean function
purge : clean
	$(RM) pingpong-semaphore pingpong-racecond