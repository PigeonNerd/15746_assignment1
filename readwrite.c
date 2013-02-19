/* $cmuPDL: readwrite.c,v 1.3 2010/02/27 11:38:39 rajas Exp $ */

/* readwrite.c
 * 
 * Code to read and write sectors to a "disk" file.
 * This is a support file for the "fsck" storage systems laboratory.
 * 
 * author: Yingchao Liu
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>	/* for memcpy() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include "fsck.h"
#include "ext2_fs.h"
#include "genhd.h"

//#define __FreeBSD__ 1
#if defined(__FreeBSD__)
#define lseek64 lseek
#endif

# define lotsOfZeros 446

/* linux: lseek64 declaration needed here to eliminate compiler warning. */
extern int64_t lseek64(int, int64_t, int);
const unsigned int sector_size__bytes = 512;
const unsigned int block_size_bytes = 1024;

int device;  /* disk file descriptor */

/* print_sector: print the contents of a buffer containing one sector.
 *
 * inputs:
 *   char *buf: buffer must be >= 512 bytes.
 *
 * outputs:
 *   the first 512 bytes of char *buf are printed to stdout.
 *
 * modifies:
 *   (none)
 */
void
print_sector (unsigned char *buf)
{
  int i;
  for (i=0; i<sector_size__bytes; i++) {
    printf("%02x", buf[i]);
    if (!((i+1)%32)) printf("\n");	/* line break after 32 bytes */
    else if (!((i+1)%4)) printf(" ");	/* space after 4 bytes */
  }
}


/* read_sectors: read a specified number of sectors into a buffer.
 *
 * inputs:
 *   int64 block: the starting sector number to read.
 *                sector numbering starts with 0.
 *   int numsectors: the number of sectors to read.  must be >= 1.
 *   int device [GLOBAL]: the disk from which to read.
 *
 * outputs:
 *   void *into: the requested number of blocks are copied into here.
 *
 * modifies:
 *   void *into
 */
void
read_sectors (int64_t block, unsigned int numsectors, void *into)
{
  int ret;
  int64_t lret;

  // if (1) {
  //     if (numsectors == 1) {
  //       printf("Reading sector %"PRId64"\n", block);
  //     } else {
  //       printf("Reading sectors %"PRId64"--%"PRId64"\n", block, block + (numsectors-1));
  //     }
  //   }

  if ((lret = lseek64(device, block * sector_size__bytes, SEEK_SET)) 
      != block * sector_size__bytes) {
    fprintf(stderr, "Seek to position %"PRId64" failed: returned %"PRId64"\n", 
	    block * sector_size__bytes, lret);
    exit(-1);
  }
  if ((ret = read(device, into, sector_size__bytes * numsectors)) 
      != sector_size__bytes * numsectors) {
    fprintf(stderr, "Read block %"PRId64" length %d failed: returned %"PRId64"\n", 
	    block, numsectors, ret);
    exit(-1);
  }
}


/* write_sectors: write a buffer into a specified number of sectors.
 *
 * inputs:
 *   int64 block: the starting sector number to write.
 *                sector numbering starts with 0.
 *   int numsectors: the number of sectors to write.  must be >= 1.
 *   void *from: the requested number of blocks are copied from here.
 *
 * outputs:
 *   int device [GLOBAL]: the disk into which to write.
 *
 * modifies:
 *   int device [GLOBAL]
 */
void
write_sectors (int64_t block, unsigned int numsectors, void *from)
{
  int ret;
  int64_t lret;

  // if (1) {
  //    if (numsectors == 1) {
  //      printf("Reading sector  %"PRId64"\n", block);
  //    } else {
  //      printf("Reading sectors %"PRId64"--%"PRId64"\n", block, block + (numsectors-1));
  //    }
  //  }

  if ((lret = lseek64(device, block * sector_size__bytes, SEEK_SET)) 
      != block * sector_size__bytes) {
    fprintf(stderr, "Seek to position %"PRId64" failed: returned %"PRId64"\n", 
	    block * sector_size__bytes, lret);
    exit(-1);
  }
  if ((ret = write(device, from, sector_size__bytes * numsectors)) 
      != sector_size__bytes * numsectors) {
    fprintf(stderr, "Read block %"PRId64" length %d failed: returned %d\n", 
	    block, numsectors, ret);
    exit(-1);
  }
}

/*
	print partition table( including extended)
*/
void print_partition_table(unsigned char* MBR){
	int i;
	unsigned char extended[sector_size__bytes];
	for (i = 0 ; i < 4 ; i++){

	    struct partition* sp = (struct partition *)(MBR + lotsOfZeros + (16 * i));

		printf("0x%02X %d %d\n", sp->sys_ind, sp->start_sect, sp->nr_sects);
		if(sp->sys_ind == 5){
			unsigned int baseStart = sp->start_sect;
			while(sp->sys_ind == 5){
				read_sectors(baseStart, 1, extended);
				struct partition* logicalPartition = (struct partition *)(extended + lotsOfZeros);
				printf("0x%02X %d %d\n", logicalPartition->sys_ind, baseStart + logicalPartition->start_sect, logicalPartition->nr_sects);
				sp =  (struct partition *)(extended + lotsOfZeros + 16);
				baseStart += sp->start_sect;
			}
		}
	}
}

/*
	print a partition's start and end 
*/

void print_partition(unsigned char* MBR, int p){
	partition* sp;
	
	if(p < 5){
		sp = (partition *)(MBR + lotsOfZeros + 16*(p-1));
		printf("0x%02X %d %d\n", sp->type, sp->start, sp->length);
	}else{
		sp = (partition *)(MBR + lotsOfZeros + (16 * 3));
		unsigned char extended[sector_size__bytes];
		int baseStart = 0;
		int i;
		for(i=5; i <= p ; i ++){
			baseStart += sp->start;
			if(sp->type == 5){
			read_sectors(baseStart, 1, extended);
			sp =  (partition *)(extended + lotsOfZeros + 16);
			}else{
				printf("%d\n", -1);
				return;
			}
		}
		partition* logicalPartition = (partition *)(extended + lotsOfZeros);
		printf("0x%02X %d %d\n", logicalPartition->type, baseStart + logicalPartition->start, logicalPartition->length);
	}
}

/*
	print superBlock for partition 1
*/

void print_superBlock(unsigned char* superBlock){

	//struct ext2_super_block* sb = (struct ext2_super_block *)superBlock;
	//short magicNum = (short)sb->s_magic;
	short magicNum = *((short *)(superBlock + 56 ));
	int totalBlocks = *((int *)(superBlock + 32));

	printf("magic number: 0x%02X\n", magicNum);
	printf("total number of blocks: %d\n", totalBlocks);
}

/*
	translate from inode number to sector number
*/

int inodeToSector(unsigned char* superBlock, int baseSector, int inodeNum){
	int inodesPerBlockGroup = *((int* )(superBlock + 40));
	int groupIndex = (inodeNum - 1 )/inodesPerBlockGroup;

	unsigned char descriptorTable[block_size_bytes];

	read_sectors(baseSector + 4, 2, descriptorTable);

	//tmp define
	int decriptorSize = 32; // bytes
	int blockId = *(int* )(descriptorTable + groupIndex * decriptorSize + 8);



	printf("number of inodes per block group: %d\n", inodesPerBlockGroup);
	printf(" blockId: %d\n", blockId);
	return 0;
}



int
main (int argc, char **argv)
{
  
	unsigned char buf[sector_size__bytes];	/* temporary buffer */
	int           the_sector;			/* IN: sector to read */
	char* 		  disk;
	char 		  op;
	while((op = getopt(argc, argv, "i:p:")) != EOF){
		switch(op){
			case 'i':
				disk = optarg;
				break;
			case 'p':
				the_sector = atoi(optarg);
				break;
		}
	}

	if ((device = open(disk, O_RDWR)) == -1) {
	 perror("Could not open device file");
	 exit(-1);
	}

	// here we read the master boot record
	read_sectors(0, 1, buf);
	print_partition_table(buf);
	printf("_____________________________\n");
	print_partition(buf, the_sector);


	unsigned char superBlockOfPart1[ 4*sector_size__bytes ];
	partition* part1 = (partition *)(buf + lotsOfZeros);
	read_sectors(part1->start + 2, 2, superBlockOfPart1);
	print_superBlock(superBlockOfPart1);

	inodeToSector(superBlockOfPart1 , part1->start, 2009);

	close(device);
	return 0;
}

/* EOF */
