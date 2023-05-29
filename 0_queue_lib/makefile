# Flags utilized
DEBUG_FLAGS = -g
CFLAGS = -std=gnu99 -Wall

# Declaring calls to clean unused files
.PHONY : all clean

# Main file
all : test_queue
# Compilation rule for main file
test_queue : test_queue.o queue.o
	gcc $(CFLAGS) -o $@ $^
# Compilation with debug rule for main file
debug : test_queue.o queue.o
	gcc $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

# Declaring clean function
clean :
	$(RM) *.o
# Declaring clean function
purge : clean
	$(RM) test_queue