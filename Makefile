# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -DDEBUG
LDFLAGS		= -lm
OBJS		= readwrite.o
BINS        = myfsck
K = 3

# Explit build and testing targets

all: ${BINS}

run: myfsck
	./myfsck -p ${K} -i disk
	rm -f *.o $(BINS)
myfsck: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(BINS)

readwrite.o: ext2_fs.h fsck.c
	${CC} fsck.c ${CFLAGS} -c -o $@

