# Flags utilized
DEBUG_FLAGS = -g -DDEBUG
CFLAGS = -std=gnu99 -Wextra

# Declaring calls to debug and clean unused files
.PHONY : all clean debug

# Main test file
all : pingpong-scheduler
# Compilation rule for the Main test file
pingpong-scheduler: ppos_core.o pingpong-scheduler.o queue.o
	gcc -o $@ $^
# Pattern rule that matches any .o file and its corresponding .c file.
%.o: %.c
	gcc $(CFLAGS) -c $^

# Debug rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: pingpong-scheduler

# Declaring clean function
clean :
	$(RM) *.o *.log
# Declaring purge clean function
purge : clean
	$(RM) pingpong-scheduler