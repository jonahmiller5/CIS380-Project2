CC=clang
CFLAGS= -g -Wall
FS_DEPS = fs/filesystem.c fs/descriptors.c fs/directory.c fs/fat.c errors.c kernel_functions.c pcb.c
TARGETS = PennOS mkFlatFAT lsFlatFAT catFlatFAT

all: clean $(TARGETS) # will change in final to change to PennOS -- recommend file system makes own make file commands

default: clean $(TARGETS)

PennOS: shell.o kernel_functions.o user_functions.o pcb.o filesystem.o descriptors.o directory.o fat.o errors.o
	clang -o PennOS shell.o user_functions.o kernel_functions.o pcb.o jobs.o tokenizer.o \
	filesystem.o descriptors.o errors.o directory.o fat.o

shell.o: shell.c shell.h
	clang -c -Wall shell.c user_functions.c jobs.c tokenizer.c $(FS_DEPS)

pcb.o: pcb.c pcb.h
	clang -c -Wall pcb.c 

tokenizer.o: tokenizer.c tokenizer.h
	clang -c -Wall tokenizer.c

jobs.o: jobs.c jobs.h
	clang -c -Wall jobs.c

kernel_functions.o: kernel_functions.c kernel_functions.h
	clang -c -Wall kernel_functions.c pcb.c

user_functions.o: user_functions.c user_functions.h
	clang -c -Wall user_functions.c kernel_functions.c

filesystem.o: fs/filesystem.c fs/filesystem.h
	$(CC) $(CFLAGS) fs/filesystem.c fs/descriptors.c fs/directory.c fs/fat.c \
	kernel_functions.c errors.c pcb.c

descriptors.o: fs/descriptors.c fs/descriptors.h
	$(CC) $(CFLAGS) fs/descriptors.c fs/directory.c fs/fat.c

directory.o: fs/directory.c fs/directory.h
	$(CC) $(CFLAGS) fs/directory.c fs/fat.c

fat.o: fs/fat.c fs/fat.h
	$(CC) $(CFLAGS) fs/fat.c

errors.o: errors.c errors.h
	$(CC) $(CFLAGS) errors.c

mkFlatFAT:
	$(CC) $(CFLAGS) execs/mkFlatFAT.c -o $@

lsFlatFAT:
	$(CC) $(CFLAGS) execs/lsFlatFAT.c $(FS_DEPS) -o $@

catFlatFAT:
	$(CC) $(CFLAGS) execs/catFlatFAT.c $(FS_DEPS) -o $@

clean:
	rm -rf *.o $(TARGETS)
