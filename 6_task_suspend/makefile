# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# All tests files
all : pingpong-wait
# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Compilation rule for the preemption stress file
pingpong-wait: ppos_core.o pingpong-wait.o queue.o
	gcc -o $@ $^

# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Declaring clean function
clean :
	$(RM) *.o *.log
# Declaring purge clean function
purge : clean
	$(RM) pingpong-wait