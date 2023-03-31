# Flags utilized
DEBUG_FLAGS = -g
CFLAGS = -std=gnu99 -Wextra

# Declaring calls to clean unused files
.PHONY : all clean

# Main tests files
all : test1 test2 test3
# Compilation rule for first test file
test1 : test1.o ppos_core.o queue.o ppos_data.h ppos.h
	gcc $(CFLAGS) -o $@ $^
# Compilation rule for second test file
test2 : test2.o ppos_core.o queue.o ppos_data.h ppos.h
	gcc $(CFLAGS) -o $@ $^
# Compilation rule for third test file
test3 : test3.o ppos_core.o queue.o ppos_data.h ppos.h
	gcc $(CFLAGS) -o $@ $^
# Compilation rule for ppos_core
ppos_core.o : ppos_core.c
	gcc $(CFLAGS) -c $^
# Compilation rule for queue
queue.o : queue.c
	gcc $(CFLAGS) -c $^

# Declaring clean function
clean :
	$(RM) *.o
# Declaring purge clean function
purge : clean
	$(RM) test1 test2 test3
