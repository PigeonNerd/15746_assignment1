#ifndef _FSCK_H
#define _FSCK_H

typedef struct pat{
	char active;
	char begin[3];
	char type;
	char end[3];
	int start;
	int length;
} partition;
#endif