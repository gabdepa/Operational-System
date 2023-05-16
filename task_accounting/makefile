# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# All tests files
all : pingpong-preemption pingpong-preemption-stress

# Compilation rule for the preemption stress file
pingpong-preemption-stress: ppos_core.o pingpong-preemption-stress.o queue.o
	gcc -o $@ $^

# Compilation rule for the preemption file
pingpong-preemption: ppos_core.o pingpong-preemption.o queue.o
	gcc -o $@ $^

# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: pingpong-preemption pingpong-preemption-stress

# Declaring clean function
clean :
	$(RM) *.o *.log
# Declaring purge clean function
purge : clean
	$(RM) pingpong-preemption pingpong-preemption-stress