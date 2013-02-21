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
#include "ext2_fs.h"
#include "genhd.h"

//#define __FreeBSD__ 1
#if defined(__FreeBSD__)
#define lseek64 lseek
#endif

#define lotsOfZeros 446

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

void readBuf(int64_t block, size_t len, void* into){
  int ret;
  int64_t lret;
  if ((lret = lseek64(device, block, SEEK_SET)) 
      != block) {
    fprintf(stderr, "Seek to position %"PRId64" failed: returned %"PRId64"\n", 
	    block, lret);
    exit(-1);
  }
  if ((ret = read(device, into, len )) 
      != len) {
    fprintf(stderr, "Read block %"PRId64" length %d failed: returned %"PRId64"\n", 
	    block,ret);
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

int64_t print_partition(unsigned char* MBR, int p){
	struct partition* sp;
	
	if(p < 5){
		sp = (struct partition *)(MBR + lotsOfZeros + 16*(p-1));
		printf("0x%02X %d %d\n", sp->sys_ind, sp->start_sect, sp->nr_sects);
    return sp->start_sect;
	}else{
		int i;
	    for(i = 0; i < 5; i++){
            sp = (struct partition *)(MBR + lotsOfZeros + 16 * i);
            if(sp->sys_ind == 5){
                break;
            }
        }
        unsigned char extended[sector_size__bytes];
		unsigned int baseStart = sp->start_sect;
		for(i=5; i <= p ; i ++){
  		    	read_sectors(baseStart, 1, extended);
  			    sp =  (struct partition *)(extended + lotsOfZeros + 16);
			if(sp->sys_ind == 5 && i != p){
                baseStart += sp->start_sect;
            }else if( sp->sys_ind != 5 && i !=p ){
				printf("%d\n", -1);
				return sp->start_sect;
			}
		}
		struct partition* logicalPartition = (struct partition *)(extended + lotsOfZeros);
		printf("0x%02X %d %d\n", logicalPartition->sys_ind, baseStart +  logicalPartition->start_sect, logicalPartition->nr_sects);
	  return sp->start_sect;
  }
}

/*
    read the super block for a partition
*/
void read_superBlock(unsigned int baseSector, 
        struct ext2_super_block* superBlock){
  read_sectors(baseSector + 2, 2, (void* )superBlock);
}

/*
    read the block descriptor
*/
void read_blockDesc(unsigned int baseSector, int groupIndex, 
    struct ext2_group_desc* descriptor){
    unsigned int start = (baseSector + 4)*sector_size__bytes 
      + groupIndex * sizeof(struct ext2_group_desc);
    readBuf(start, sizeof(struct ext2_group_desc), (void *) descriptor);
}

/*
    read the inode bit map for a certain group 
 */
void read_inode_bitMap(unsigned int baseSector, 
    struct ext2_group_desc* descriptor, unsigned char* bitMap){
    unsigned int blockId = descriptor->bg_inode_bitmap;
    read_sectors(baseSector + blockId * 2, 2, bitMap);
}

/*
    read the block bit map for a certain group
 */
void read_block_bitMap(unsigned int baseSector, 
    struct ext2_group_desc* descriptor, unsigned char* bitMap){
    unsigned int blockId = descriptor->bg_block_bitmap;
    read_sectors(baseSector + blockId * 2, 2, bitMap);
}

/*
    read inode based on inode number
 */
void  read_inode(struct ext2_super_block* superBlock, 
    unsigned int baseSector, int inodeNum, struct ext2_inode* inode){
    unsigned int inodesPerBlockGroup = superBlock->s_inodes_per_group;
    int groupIndex = (inodeNum - 1 )/inodesPerBlockGroup;
    int indexInInodeTable = (inodeNum - 1 )%inodesPerBlockGroup;  
    printf("group index: %d, inode table index %d\n", groupIndex, indexInInodeTable);
    struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // This is beginning of the inode table
    unsigned int blockId = thisDesc.bg_inode_table;
    unsigned int startBytes = baseSector * sector_size__bytes + 
            blockId * block_size_bytes + (indexInInodeTable)* sizeof(struct ext2_inode); 
    readBuf(startBytes, sizeof(struct ext2_inode), (void*)inode);
}

/*
	print superBlock
*/
void print_superBlock(struct ext2_super_block* superBlock){
  	unsigned char magicNum = superBlock->s_magic;
  	unsigned int totalBlocks = superBlock->s_blocks_count;
  	unsigned int totalInodes = superBlock->s_inodes_count;
      unsigned int inodesPerBlockGroup = superBlock->s_inodes_per_group;
  	unsigned int blocksPerBlockGroup = superBlock->s_blocks_per_group;
  	printf("magic number: 0x%02X\n", magicNum);
  	printf("total number of blocks: %d\n", totalBlocks);
  	printf("total number of inodes: %d\n", totalInodes);
    printf("number of inodes per block group: %d\n", inodesPerBlockGroup);
    printf("number of blocks per block group: %d\n", blocksPerBlockGroup);
}

/*
	translate from inode number to sector number
*/

int inodeToSector(struct ext2_super_block* superBlock, unsigned int baseSector, int inodeNum){
  	unsigned int inodesPerBlockGroup = superBlock->s_inodes_per_group;
  	int groupIndex = (inodeNum - 1 )/inodesPerBlockGroup;
  	int indexInInodeTable = (inodeNum - 1 )%inodesPerBlockGroup;
  	printf("group index: %d, inode table index %d\n", groupIndex, indexInInodeTable);
  	struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // This is beginning of the inode table
    unsigned int blockId = thisDesc.bg_inode_table;
  	unsigned int totalSizeInbytes = blockId * block_size_bytes + (indexInInodeTable)* sizeof(struct ext2_inode);
    printf("Inode table is at block %d\n", blockId);
    int numSectors = totalSizeInbytes/sector_size__bytes;
  	printf("number of sectors is: %d\n", numSectors);
  	return baseSector + numSectors;
}

/*
 * test the inode is set inode bitmap 
 */
int isInodeInBitMap(struct ext2_super_block* superBlock, int inodeNum, unsigned int baseSector){
    unsigned int inodesPerBlockGroup = superBlock->s_inodes_per_group;
  	int groupIndex = (inodeNum - 1 )/inodesPerBlockGroup;
  	int indexInInodeTable = (inodeNum - 1 )%inodesPerBlockGroup;
    struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    unsigned char bitMap[block_size_bytes];
    read_inode_bitMap(baseSector, &thisDesc, bitMap);
    print_sector(bitMap); 
    int byteOffset = indexInInodeTable/8;
    int bitOffset = indexInInodeTable%8;
    printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    unsigned char thisByte = bitMap[byteOffset];
    return thisByte && (1<<bitOffset);
}
/*
 *  test if blocks are set in block bitmap
 */
int isBlockInBitMap(struct ext2_super_block* superBlock, unsigned int blockId,
        unsigned int baseSector){
    printf("Test block %d...................\n", blockId);
    unsigned int blocksPerGroup = superBlock->s_blocks_per_group;
    int groupIndex = blockId/blocksPerGroup;
	struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    unsigned int bitMapBlockId = thisDesc.bg_block_bitmap;
    // TODO: this needs rethinking
    int offset = blockId - bitMapBlockId - 216; 
	printf("OFFSET IS %d\n", offset);
    unsigned char bitMap[block_size_bytes];
    read_block_bitMap(baseSector, &thisDesc, bitMap);
    int byteOffset = offset/8;
    int bitOffset = offset%8;
    //printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    unsigned char thisByte = bitMap[byteOffset];
    return thisByte && (1<<bitOffset);
}

/*
    get inode number for the target file/dir
    if not exist, return -1;
 */
int getInodeNumBasedOnPath(struct ext2_inode* inode, unsigned int baseSector, char* path){
    // here we actually need to go through every block including indirect block
    unsigned int blockId = inode->i_block[0];
    unsigned char block[2*sector_size__bytes];
    read_sectors(baseSector + blockId*2, 2, block);
    struct ext2_dir_entry_2* entry = (struct ext2_dir_entry_2* )block;
    int size;
    size = 0;
    while(size < inode->i_size && entry->inode != 0){
        char file_name[EXT2_NAME_LEN +1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        if(strncmp(file_name, path, strlen(path)) == 0){
            printf("Found it ! %10u %s\n", entry->inode, file_name);
            return entry->inode;
        }
        entry = (void*)entry + entry->rec_len;
        size += entry->rec_len;
    }
    return -1;
}
void findInodeBasedOnPath(struct ext2_super_block* superBlock,
         unsigned int baseSector, char path [], struct ext2_inode* inode){
    path++;
    int inodeNum;
    struct ext2_inode thisInode;
    read_inode(superBlock, baseSector, 2, &thisInode);
    char* token = strtok(path, "/");
    while(token != NULL){
        printf("Target: %s\n", token);
        inodeNum = getInodeNumBasedOnPath(&thisInode, baseSector, token);
        if(inodeNum){
          read_inode(superBlock, baseSector, inodeNum, &thisInode);
          token = strtok(NULL,"/");
        }else{
          return;
        }
    }
    memcpy(inode, &thisInode, sizeof(struct ext2_inode));
}

void part2Test(){
    printf("____________________Below is for tesing purpose___________________\n");
    unsigned char MBR[sector_size__bytes];
    read_sectors(0, 1, MBR);
    print_partition_table(MBR);
    int64_t baseSector = print_partition(MBR, 1);
    struct ext2_super_block superBlock;
    read_superBlock(baseSector, &superBlock);
    print_superBlock(&superBlock);
    struct ext2_inode rootInode;
    read_inode(&superBlock, baseSector, 2, &rootInode);
    if(S_ISDIR(rootInode.i_mode)){
      printf("This is dir\n");
    }else{
      printf("This is junk\n");
    }
    int yes =  isInodeInBitMap(&superBlock, 2, baseSector);
    if(yes){
      printf("root inode is set\n");
    }
    struct ext2_inode targetInode;
    char path[] = "/lions/tigers/bears/ohmy.txt";  
    findInodeBasedOnPath(&superBlock, baseSector, path, &targetInode);
    unsigned int blockId = targetInode.i_block[0];
    yes = isBlockInBitMap(&superBlock, blockId, baseSector);
    if(yes){
      printf("block %d is set\n", blockId);
    }
}


int
main (int argc, char **argv)
{
	unsigned char MBR[sector_size__bytes];	/* temporary buffer */
	int           partitionToRead;			/* IN: partition to read */
	char* 		  disk;
	char 		  op;
	while((op = getopt(argc, argv, "i:p:")) != EOF){
		switch(op){
			case 'i':
				disk = optarg;
				break;
			case 'p':
				partitionToRead = atoi(optarg);
				break;
            case 'f':
                break;
        }
	}
	if ((device = open(disk, O_RDWR)) == -1) {
	 perror("Could not open device file");
	 exit(-1);
	}   
  //part2Test();
  
    
    read_sectors(0, 1, MBR);
    print_partition(MBR, partitionToRead);
    close(device);
	return 0;
}

/* EOF */
