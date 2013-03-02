# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -DDEBUG
LDFLAGS		= -lm
OBJS		= readwrite.o
BINS        = myfsck
K = 6 

# Explit build and testing targets

all: ${BINS}

run: myfsck
	./myfsck -p ${K} -i disk_original
	rm -f *.o $(BINS)
fix: myfsck
	rm disk
	cp disk_original disk
	./myfsck -f ${K} -i disk
	rm -f *.o $(BINS)
myfsck: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
submit:
	tar -cvf myfsck.tar *.c *.h Makefile 
clean:
	rm -f *.o $(BINS)

readwrite.o: ext2_fs.h fsck.c
	${CC} fsck.c ${CFLAGS} -c -o $@

