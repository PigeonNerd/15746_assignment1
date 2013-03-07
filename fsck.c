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
#define minOfTwo(x, y) ((x > y) ? y : x)
#define CHECK_REFERENCED 1
#define COUNT_LINK 2


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

   //if (1) {
   //    if (numsectors == 1) {
   //      printf("Reading sector %"PRId64"\n", block);
   //    } else {
   //      printf("Reading sectors %"PRId64"--%"PRId64"\n", block, block + (numsectors-1));
    //   }
    // }

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
	struct partition* logicalPartition; 
	if(p < 5){
		sp = (struct partition *)(MBR + lotsOfZeros + 16*(p-1));
		printf("0x%02X %d %d\n", sp->sys_ind, sp->start_sect, sp->nr_sects);
        if(sp->sys_ind == 131){
            return sp->start_sect;
        }else{
            return 0;
        }
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
		unsigned int extendedStart = baseStart; 
        unsigned int logiStart = 0;
        for(i=5; i <= p ; i ++){
  		    	read_sectors(extendedStart, 1, extended);
		        logicalPartition = (struct partition *)(extended + lotsOfZeros);
                logiStart = extendedStart + logicalPartition->start_sect;
                sp =  (struct partition *)(extended + lotsOfZeros + 16);
                //printf("type %d EXTENDED @ %d, ", sp->sys_ind, sp->start_sect);
                //printf("type %d LOGICAL @ %d ---------",logicalPartition->sys_ind, logicalPartition->start_sect);
			if(sp->sys_ind == 5 && i != p){
                extendedStart = baseStart + sp->start_sect;
            }else if( sp->sys_ind != 5 && i !=p ){
				printf("%d\n", -1);
				return sp->start_sect;
			}
		}
		printf("0x%02X %d %d\n", logicalPartition->sys_ind, logiStart, logicalPartition->nr_sects);
        if(logicalPartition->sys_ind == 131){
            return logiStart;
        }else{
            return 0;
        }
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
    
    //printf("group index: %d, inode table index %d\n", groupIndex, indexInInodeTable);
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
  	//printf("group index: %d, inode table index %d\n", groupIndex, indexInInodeTable);
  	struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // This is beginning of the inode table
    unsigned int blockId = thisDesc.bg_inode_table;
  	unsigned int totalSizeInbytes = blockId * block_size_bytes + (indexInInodeTable)* sizeof(struct ext2_inode);
    //printf("Inode table is at block %d\n", blockId);
    int numSectors = totalSizeInbytes/sector_size__bytes;
  	//printf("number of sectors is: %d\n", numSectors);
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
    //print_sector(bitMap);
    int byteOffset = indexInInodeTable/8;
    int bitOffset = indexInInodeTable%8;
    //printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    unsigned char thisByte = bitMap[byteOffset];
    //printf("%2x\n", thisByte);
    int bit = thisByte & (1<< (bitOffset));
    return bit;
}

/*
 *  test if blocks are set in block bitmap
 */
int isBlockInBitMap(struct ext2_super_block* superBlock, unsigned int blockId,
        unsigned int baseSector){
    //printf("Test block %d...................\n", blockId);
    unsigned int blocksPerGroup = superBlock->s_blocks_per_group;
    int groupIndex = (blockId - 1)/blocksPerGroup;
	struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // TODO: this needs rethinking
    int offset = (blockId - 1) % superBlock->s_blocks_per_group; 
	//printf("OFFSET IS %d\n", offset);
    unsigned char bitMap[block_size_bytes];
    read_block_bitMap(baseSector, &thisDesc, bitMap);
    int byteOffset = offset/8;
    int bitOffset = offset%8;
    //printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    unsigned char thisByte = bitMap[byteOffset];
    int bit = thisByte & (1<< (bitOffset));
    return bit;
}

/*
 *  set block bit map
 */
void setBlockBitMap(struct ext2_super_block* superBlock,
 unsigned int baseSector, unsigned int blockId, unsigned char* allBitMaps){
    if ( !isBlockInBitMap(superBlock, blockId, baseSector)){
        printf("ERROR: block %d should be set in the original bit map\n", blockId);
    }
    unsigned int startPoint[3]= {0, 1024, 2048};
    unsigned int blocksPerGroup = superBlock->s_blocks_per_group;
    int groupIndex = (blockId-1)/blocksPerGroup;
    struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // need rethink about this
    int offset = (blockId-1) % superBlock->s_blocks_per_group;
    unsigned char* bitMap = allBitMaps + startPoint[groupIndex];
    int byteOffset = offset/8;
    int bitOffset = offset%8;
    //printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    //int bit = bitMap[byteOffset] & (1<< (bitOffset));
    // if it is not present, we set it , to avoid hard link
    //if( bit == 0){
       bitMap[byteOffset] |= (1<< (bitOffset));
      // return 1;
    //}else{
      //return 0;
    //}   
}

/*
 *  test hard link
 */
int testHardLink(struct ext2_super_block* superBlock,
 unsigned int baseSector, unsigned int blockId, unsigned char* allBitMaps){
    unsigned int startPoint[3]= {0, 1024, 2048};
    unsigned int blocksPerGroup = superBlock->s_blocks_per_group;
    int groupIndex = (blockId - 1)/blocksPerGroup;
    struct ext2_group_desc thisDesc;
    read_blockDesc(baseSector, groupIndex, &thisDesc);
    // need rethink about this
    int offset = (blockId - 1)% superBlock->s_blocks_per_group;
    unsigned char* bitMap = (unsigned char*)(allBitMaps + startPoint[groupIndex]);
    int byteOffset = offset/8;
    int bitOffset = offset%8;
    //printf("byte offset is %d, bit offset is %d\n", byteOffset, bitOffset);
    int bit = bitMap[byteOffset] & (1<< (bitOffset));
    // if it is not present, we set it , to avoid hard link
    return bit;
}

/*
    read direct block
*/
void read_direct_blocks(unsigned int baseSector, unsigned int* blocks, 
                                      int numBlocks, unsigned char* current){
    int num_direct_blocks = minOfTwo(numBlocks, 12);
    int blockIndex;
    for(blockIndex = 0 ; blockIndex < num_direct_blocks; blockIndex++) {
      unsigned int blockId = blocks[blockIndex];
      read_sectors(baseSector + blockId*2, 2, current);
      current += block_size_bytes;
    }
}

/*
    read single indirect block
*/
void read_single_indirect_blocks(unsigned int baseSector, unsigned int single_inDirectBlockId, 
                                      int* numBlocks, unsigned char* current){
      
    unsigned char single_inDirectBlock[block_size_bytes];
    read_sectors(baseSector + single_inDirectBlockId*2, 2, single_inDirectBlock);
    int indirectBlockIndex;
    for(indirectBlockIndex = 0; indirectBlockIndex < block_size_bytes/4; indirectBlockIndex++){
        unsigned int blockId = *(unsigned int *)((void*)single_inDirectBlock + indirectBlockIndex * 4);
        read_sectors(baseSector + blockId * 2, 2, current);
        current += block_size_bytes;
        (*numBlocks)--;
        if(*numBlocks == 0){
          return;
        }
      }
}

/*
    read double indirect block

*/
void read_double_indirect_blocks(unsigned int baseSector, unsigned int double_inDirectBlockId, 
                                      int* numBlocks, unsigned char* current){
    
    unsigned char double_inDirectBlock[block_size_bytes];
    read_sectors(baseSector + double_inDirectBlockId * 2, 2, double_inDirectBlock);
    int doubleIndirectBlockIndex;
    for(doubleIndirectBlockIndex = 0; doubleIndirectBlockIndex < block_size_bytes/4; doubleIndirectBlockIndex++){
      unsigned int single_inDirectBlockId = *(unsigned int*)((void*)double_inDirectBlock + doubleIndirectBlockIndex * 4);
      read_single_indirect_blocks(baseSector, single_inDirectBlockId, numBlocks, current);
      if(*numBlocks == 0){
        return;
      }
    }
}

/*
    read tripple indirect block
*/
void read_tripple_indirect_blocks(unsigned int baseSector, unsigned int tripple_inDirectBlockId, 
                                      int* numBlocks, unsigned char* current){
      unsigned char tripple_inDirectBlock[block_size_bytes];
      read_sectors(baseSector + tripple_inDirectBlockId * 2, 2, tripple_inDirectBlock);
      int trippleIndirectBlockIndex;
      for(trippleIndirectBlockIndex = 0; trippleIndirectBlockIndex < block_size_bytes/4; trippleIndirectBlockIndex++){
        unsigned int double_inDirectBlockId = *(unsigned int*)((void*)tripple_inDirectBlock + trippleIndirectBlockIndex * 4);
        read_double_indirect_blocks(baseSector, double_inDirectBlockId, numBlocks, current);
        if(*numBlocks == 0){
          return;
        }
      }
}


void fetch_all_blocks(unsigned int baseSector, unsigned int blocks[], 
                                      int numBlocks, unsigned char* bigBuffer){
      unsigned char* current = bigBuffer;
      read_direct_blocks(baseSector, blocks, numBlocks, current);
      // here we go into the single indirect block
      int blocksLeft = numBlocks - 12;
      
      if(blocksLeft > 0){
        read_single_indirect_blocks(baseSector, blocks[12], &blocksLeft, current);
      }
      // here we go into the double indirect block  
      if(blocksLeft > 0){
        read_double_indirect_blocks(baseSector, blocks[13], &blocksLeft, current);
        }

      // here we go into the tripple indirect block   -------- very rare
      if(blocksLeft > 0){
        read_tripple_indirect_blocks(baseSector, blocks[14], &blocksLeft, current);
      }
    }


/*
 *  check direct blocks
 */
void check_direct_blocks(struct ext2_super_block* superBlock, 
  unsigned int baseSector, unsigned int directBlockId, unsigned char* allBitMaps){

    setBlockBitMap(superBlock, baseSector, directBlockId, allBitMaps);
}

/*
 *  check singly indirect blocks
 */
void check_singly_indirect_blocks(struct ext2_super_block* superBlock, 
  unsigned int baseSector,unsigned int single_inDirectBlockId, 
                                      int* numBlocks, unsigned char* allBitMaps){
    setBlockBitMap(superBlock, baseSector, single_inDirectBlockId, allBitMaps);
    (*numBlocks)--;
    unsigned char single_inDirectBlock[block_size_bytes];
    read_sectors(baseSector + single_inDirectBlockId*2, 2, single_inDirectBlock);
    int indirectBlockIndex;
    for(indirectBlockIndex = 0; indirectBlockIndex < block_size_bytes/4; indirectBlockIndex++){
        unsigned int directBlockId = *(unsigned int *)((void*)single_inDirectBlock + indirectBlockIndex * 4);
        check_direct_blocks(superBlock, baseSector, directBlockId, allBitMaps);
        (*numBlocks)--;
        if(*numBlocks == 0){
          return;
        }
      }
}

/*
 *  check doubly indirect blocks
 */
void check_doublely_indirect_blocks(struct ext2_super_block* superBlock, 
  unsigned int baseSector,unsigned int double_inDirectBlockId, 
                                      int* numBlocks, unsigned char* allBitMaps){
    setBlockBitMap(superBlock, baseSector, double_inDirectBlockId, allBitMaps);
    (*numBlocks)--;
    unsigned char double_inDirectBlock[block_size_bytes];
    read_sectors(baseSector + double_inDirectBlockId * 2, 2, double_inDirectBlock);
    int doubleIndirectBlockIndex;
    for(doubleIndirectBlockIndex = 0; doubleIndirectBlockIndex < block_size_bytes/4; doubleIndirectBlockIndex++){
      unsigned int single_inDirectBlockId = *(unsigned int*)((void*)double_inDirectBlock + doubleIndirectBlockIndex * 4);  
      check_singly_indirect_blocks(superBlock, baseSector, single_inDirectBlockId, numBlocks, allBitMaps);
      if(*numBlocks == 0){
        return;
      }
    }
}

/*
 *  check tripple indirect blocks
 */
void check_tripple_indirect_blocks(struct ext2_super_block* superBlock, 
  unsigned int baseSector,unsigned int tripple_inDirectBlockId, 
                                      int* numBlocks, unsigned char* allBitMaps){
      setBlockBitMap(superBlock, baseSector, tripple_inDirectBlockId, allBitMaps);
      (*numBlocks)--;
      unsigned char tripple_inDirectBlock[block_size_bytes];
      read_sectors(baseSector + tripple_inDirectBlockId * 2, 2, tripple_inDirectBlock);
      int trippleIndirectBlockIndex;
      for(trippleIndirectBlockIndex = 0; trippleIndirectBlockIndex < block_size_bytes/4; trippleIndirectBlockIndex++){
        unsigned int double_inDirectBlockId = *(unsigned int*)((void*)tripple_inDirectBlock + trippleIndirectBlockIndex * 4);
        check_doublely_indirect_blocks(superBlock, baseSector, double_inDirectBlockId, numBlocks, allBitMaps);
        if(*numBlocks == 0){
          return;
        }
      }
}

/*
 *  check reg file inode blocks
 */
 void check_file_inode_blocks(struct ext2_super_block* superBlock, unsigned int baseSector, 
                int inodeNum, unsigned char* allBitMaps, int* allocateCount){

      struct ext2_inode inode;
      read_inode(superBlock, baseSector, inodeNum, &inode);
      int numBlocks = inode.i_blocks/(2<<superBlock->s_log_block_size);
      
      //printf("Start to check file inode %d with %d blocks reserved\n", inodeNum, numBlocks);
      if( testHardLink(superBlock, baseSector, inode.i_block[0], allBitMaps) ){
          printf("Find a hard link for inode %d, returns\n", inodeNum);
          return;
      }

      *allocateCount += numBlocks;
      int blocksLeft = numBlocks;
      int num_direct_blocks = minOfTwo(blocksLeft, 12);
      int i;
      // here we check the direct blocks
      for(i = 0; i < num_direct_blocks; i++) {
          unsigned int directBlockId = inode.i_block[i];
          check_direct_blocks(superBlock, baseSector, directBlockId, allBitMaps);
          blocksLeft--;
      }
      // here we check the singly indirect blocks
      if(blocksLeft > 0){
          check_singly_indirect_blocks(superBlock, baseSector, inode.i_block[12], &blocksLeft, allBitMaps);
      }
      // here we check the doubely indirect blocks
      if(blocksLeft > 0){
          check_doublely_indirect_blocks(superBlock, baseSector, inode.i_block[13], &blocksLeft, allBitMaps);
      }
      // here we check the tripple indirect blocks
      if(blocksLeft > 0){
          check_tripple_indirect_blocks(superBlock, baseSector, inode.i_block[14], &blocksLeft, allBitMaps);
      }
 }

/*
 *  check dir file inode blocks
 */
 void check_dir_inode_blocks(struct ext2_super_block* superBlock, unsigned int baseSector, 
                int inodeNum, unsigned char* allBitMaps, int* allocateCount, int* inodeCount){
      struct ext2_inode inode;
      read_inode(superBlock, baseSector, inodeNum, &inode);
      int numBlocks = inode.i_blocks/(2<<superBlock->s_log_block_size);
      
      //printf("Start to check dir inode %d with %d blocks reserved\n", inodeNum, numBlocks);
      *allocateCount += numBlocks;
      int blocksLeft = numBlocks;
      int num_direct_blocks = minOfTwo(blocksLeft, 12);
      int i;
      // here we check the direct blocks
      for(i = 0; i < num_direct_blocks; i++) {
          unsigned int directBlockId = inode.i_block[i];
          check_direct_blocks(superBlock, baseSector, directBlockId, allBitMaps);
          blocksLeft--;
      }
      // here we check the singly indirect blocks
      if(blocksLeft > 0){
          check_singly_indirect_blocks(superBlock, baseSector, inode.i_block[12], &blocksLeft, allBitMaps);
      }
      // here we check the doubely indirect blocks
      if(blocksLeft > 0){
          check_doublely_indirect_blocks(superBlock, baseSector, inode.i_block[13], &blocksLeft, allBitMaps);
      }
      // here we check the tripple indirect blocks
      if(blocksLeft > 0){
          check_tripple_indirect_blocks(superBlock, baseSector, inode.i_block[14], &blocksLeft, allBitMaps);
      }

      // end checking this dir, now try to go through the directory tree if any

    numBlocks = inode.i_size/block_size_bytes; // this is actual used number of blocks
    if(inode.i_size % block_size_bytes){
      numBlocks ++;
    }
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode.i_block, numBlocks, bigBufferOfBlocks);
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size = 0;
    int count = 0;
    while(size < inode.i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks  + size;
        size += entry->rec_len;
        count ++;
        if ( count > 2 && entry->file_type == EXT2_FT_DIR){
            check_dir_inode_blocks(superBlock, baseSector, entry->inode, allBitMaps, allocateCount, inodeCount);
        }
           
        else if (count > 2 && entry->file_type == EXT2_FT_REG_FILE){
            (*inodeCount) ++;
            check_file_inode_blocks(superBlock, baseSector, entry->inode, allBitMaps, allocateCount);
        }

      }
 }

/*
 *  pre - set the block bitmap
 */
void pre_set_block_bitmap(struct ext2_super_block* superBlock, unsigned int baseSector, unsigned char* allBitMaps){
     setBlockBitMap(superBlock, baseSector, 1, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 2, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 3, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 4, allBitMaps);
    int i;
    for(i = 5; i <= 255; i++){
     setBlockBitMap(superBlock, baseSector, i, allBitMaps);
    }
     setBlockBitMap(superBlock, baseSector, 8193, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 8194, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 8195, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 8196, allBitMaps);
   for(i = 8197; i <= 8447; i++){
     setBlockBitMap(superBlock, baseSector, i, allBitMaps);
   }
    setBlockBitMap(superBlock, baseSector, 16385, allBitMaps);
     setBlockBitMap(superBlock, baseSector, 16386, allBitMaps);
   for(i = 16389; i <= 16639; i++){
     setBlockBitMap(superBlock, baseSector, i, allBitMaps);
   }

    //TODO: padding the rest of the last group block bit map
   int logiTotal = 3 * superBlock->s_blocks_per_group;
   int actualTotal = superBlock->s_blocks_count;
   for(i = logiTotal; i >= actualTotal; i--){
     setBlockBitMap(superBlock, baseSector, i, allBitMaps);
    }   
}

/*
 *  compare orginal bitmap and the bitmap generated
 */
void compareBitMap(struct ext2_super_block* superBlock, unsigned int baseSector, unsigned char* allBitMaps){
   printf("Start to compare two bit maps..........\n\n"); 
	struct ext2_group_desc thisDesc;
    unsigned char bitMap[block_size_bytes];
    unsigned int startPoint[3]= {0, 1024, 2048};
    int groupIndex;
    for(groupIndex = 0; groupIndex <=2; groupIndex ++){
        read_blockDesc(baseSector, groupIndex, &thisDesc);
        read_block_bitMap(baseSector, &thisDesc, bitMap);
        unsigned char* myBitMap = allBitMaps + startPoint[groupIndex];
        int byteIndex;
        for(byteIndex = 0; byteIndex < 1024; byteIndex ++){
            if(bitMap[byteIndex] != myBitMap[byteIndex]){
                printf("ERROR: @byte  %d , original %x, mine %x\n", byteIndex,bitMap[byteIndex], myBitMap[byteIndex]);                
            }
        }
    }
   printf("\nEnd  compare two bit maps..........\n"); 
}



/*
 *  check block allocation
 */
void check_all_blocks(struct ext2_super_block* superBlock, unsigned int baseSector){
    printf("-----------------Start PASS FOUR check blocks allocation-------------\n\n");
    int numPresetBlocks = 3 * superBlock->s_inodes_per_group/(block_size_bytes/sizeof(struct ext2_inode)) + 5 + 4 + 2;
    int allocateCount = 0;
    int inodeCount = 0;
    unsigned char allBitMaps[3 * 1024] = {0};
    pre_set_block_bitmap(superBlock, baseSector, allBitMaps);
    check_dir_inode_blocks(superBlock,baseSector, 2, allBitMaps, &allocateCount, &inodeCount);
    printf("Actual number of blocks allocated: %d\n", allocateCount + numPresetBlocks);
    printf("Actual number of files %d\n", inodeCount);
    printf("Total number of blocks allocated from superblock %d\n", superBlock->s_blocks_count - superBlock->s_free_blocks_count);
    printf("Total number of inodes allocated from superblock %d\n",superBlock->s_inodes_count - superBlock->s_free_inodes_count);
    compareBitMap(superBlock, baseSector, allBitMaps);
	struct ext2_group_desc thisDesc;
    unsigned int startPoint[3]= {0, 1024, 2048};
    int groupIndex;
    unsigned int blockId; 
    for (groupIndex = 0; groupIndex <=2; groupIndex++) {
        read_blockDesc(baseSector, groupIndex, &thisDesc);
        blockId = thisDesc.bg_block_bitmap;
        unsigned char* mybitmap = allBitMaps + startPoint[groupIndex];
        write_sectors(baseSector + blockId*2, 2, mybitmap);
    }
    
    printf("-----------------End PASS FOUR--------------------\n\n");
}
/*
    auxilary fundction to print directories
 */

void print_directory(struct ext2_super_block* superBlock, struct ext2_inode* inode, unsigned int baseSector, int* fileCount, int* dirCount){
    
    int numBlocks = inode->i_size/block_size_bytes;
    if(inode->i_size%block_size_bytes){
      numBlocks ++;
    }
    printf("inode record size: %d\nnum blocks: %d\n", inode->i_size, numBlocks);
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode->i_block, numBlocks, bigBufferOfBlocks);

    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size = 0;
    int count = 0;
    while(size < inode->i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks  + size;
        size += entry->rec_len;
        count ++;
        char file_name[EXT2_NAME_LEN +1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        if( count > 2 && entry->file_type == EXT2_FT_DIR){
            (*dirCount)++;
            printf("%10u %s, type:  %d, rec len: %d\n", entry->inode, file_name, entry->file_type, entry->rec_len);
            struct ext2_inode thisInode;
            read_inode(superBlock, baseSector, entry->inode, &thisInode);
            print_directory(superBlock, &thisInode, baseSector, fileCount, dirCount);
        }else if(count > 2){
            if(entry->file_type != 1){
                printf("######## %d has file_type %d\n", entry->inode, entry->file_type);
            }
            (*fileCount)++;
            printf("%10u %s name len: %d type:  %d, rec len: %d, real len: %d\n", entry->inode, file_name,
                entry->name_len,entry->file_type, entry->rec_len, EXT2_DIR_REC_LEN(entry->name_len));
        }
    }
    printf("at the end, size is %d\n", size);
}

/*
 *  find the parent of a directory inode
 *  initially, parentNum should be -1
 */
void findParent(int inodeNum_current, int inodeNum_target, unsigned baseSector, int* parentNum){
    struct ext2_super_block superBlock;
    read_superBlock(baseSector, &superBlock);
    struct ext2_inode inode;
    read_inode(&superBlock, baseSector, inodeNum_current, &inode);
    int numBlocks = inode.i_size/block_size_bytes; 
    if(inode.i_size%block_size_bytes){
      numBlocks ++;
    }

    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode.i_block, numBlocks, bigBufferOfBlocks);
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size = 0;
    int count = 0;
    while(*parentNum < 0 && size < inode.i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks  + size;
        size += entry->rec_len;
        count ++;
        if(entry->inode == inodeNum_target){
            *parentNum = inodeNum_current;
        }else{
            if(count > 2 && entry->file_type == EXT2_FT_DIR){
              findParent(entry->inode, inodeNum_target, baseSector, parentNum);
            }
          }
        }
    }

/*
 *  check if inodeNum current is inodeNum_target's parent
 */
 int isParent(int inodeNum_current, int inodeNum_target, unsigned baseSector){
    struct ext2_super_block superBlock;
    read_superBlock(baseSector, &superBlock);
    struct ext2_inode inode;
    read_inode(&superBlock, baseSector, inodeNum_current, &inode);
    int numBlocks = inode.i_size/block_size_bytes;
    if(inode.i_size%block_size_bytes){
      numBlocks ++;
    }
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode.i_block, numBlocks, bigBufferOfBlocks);
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size = 0;
    int count = 0;
    while(size < inode.i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks  + size;
        size += entry->rec_len;
        count ++;
        if(entry->inode == inodeNum_target){
              return 1;
          }
        }
    return 0;
    }


/*
 *  check the first directory entry
 */
int check_first_entry(struct ext2_super_block* superBlock, 
            struct ext2_dir_entry_2* entry, int inodeNum, unsigned int baseSector){
    char file_name[EXT2_NAME_LEN +1];
    memcpy(file_name, entry->name, entry->name_len);
    file_name[entry->name_len] = 0;
    int needToWrite = 0;
    if(strcmp(file_name, ".") != 0){
        // make it .
        printf("NOTE: the first entry name of inode %d is not .\n", inodeNum);
        strcpy(entry->name, ".");
        entry->name_len = 1;
        //might need to adjust the size of the last entry(padding)
        needToWrite = 1;
    }
    if(entry->inode != inodeNum){
        // make it the right inode number
        printf("NOTE: the first entry of inode %d does not refer to itself\n", inodeNum);
        entry->inode = inodeNum;
        needToWrite = 1;
    }
    return needToWrite;
}

/*
 *  check the second directory entry
 */
int check_second_entry(struct ext2_super_block* superBlock, 
            struct ext2_dir_entry_2* entry, int inodeNum, unsigned int baseSector){
  char file_name[EXT2_NAME_LEN +1];
  memcpy(file_name, entry->name, entry->name_len);
  file_name[entry->name_len] = 0;
  int needToWrite = 0;
  //printf("This inode: %d, parent inode: %d\n", inodeNum, entry->inode);
  if(strcmp(file_name, "..") != 0){
    // make it ..
    printf("NOTE: the second entry name of inode %d is not .. \n", inodeNum);
    strcpy(file_name, "..");
    entry->name_len = 2;
    needToWrite = 1;
  }
  if(inodeNum == 2){
    if(entry->inode != 2){
    // make it the right number
        printf("NOTE: the second entry of root inode does refer to itself\n");
        entry->inode = 2;
        needToWrite = 1;
    }
  }else{
    if (( entry->inode > superBlock->s_inodes_count ) || (!isParent(entry->inode, inodeNum, baseSector)) ){
      int parentNum = -1;
      findParent(2, inodeNum, baseSector, &parentNum);
      // use parentNum 
        printf("NOTE: the second entry(%s) of inode %d does not refer to its parent\n", file_name, inodeNum);
        printf("Its parent should be inode: %d\n", parentNum);
        if(parentNum){
            entry->inode = parentNum;
            needToWrite = 1;
        }
    }
  }
    return needToWrite;
}

/*
 *  check the consistency of the directory
 *  fix the error if needed
 */
void checkDirectory(struct ext2_super_block* superBlock, int inodeNum, unsigned int baseSector){
    struct ext2_inode inode;
    read_inode(superBlock, baseSector, inodeNum, &inode);
    int numBlocks = inode.i_size/block_size_bytes;
    if(inode.i_size % block_size_bytes){
      numBlocks++;
    }
    //printf("inode record size: %d\nnum blocks: %d\n", inode.i_size, numBlocks);
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode.i_block, numBlocks, bigBufferOfBlocks);

    // this is the first entry 
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    // here means the first entry is not correct 
    struct ext2_dir_entry_2* entry2 = (void*)entry + entry->rec_len;
    int size = entry->rec_len + entry2->rec_len;
    if(check_first_entry(superBlock, entry, inodeNum, baseSector) || 
            check_second_entry(superBlock, entry2, inodeNum, baseSector)){
        int blockId = inode.i_block[0];
        int sectorNum = baseSector + blockId * 2;
        write_sectors(sectorNum, 2, bigBufferOfBlocks);
    }
    entry = (void*)entry2 + entry2->rec_len;
    while(size < inode.i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks + size;
        size += entry->rec_len;
       if (entry->file_type == EXT2_FT_DIR){
            checkDirectory(superBlock, entry->inode, baseSector);
       }
    }
}

/*
 *  print out all of the directory of this file system
 */
void print_all_directory(struct ext2_super_block* superBlock, unsigned int baseSector){
    struct ext2_inode rootInode;
    read_inode(superBlock, baseSector, 2, &rootInode);
    int fileCount = 0;
    int dirCount = 0;
    print_directory(superBlock, &rootInode, baseSector, &fileCount, &dirCount);
    printf("file: %d, dir: %d\n", fileCount, dirCount);
}

/*
 *  find the link count for the gievn inode, if the flag is check if reference
 *  return immediately
 */
void find_link_count(struct ext2_super_block* superBlock, int inodeNum, 
        int inodeNum_target, unsigned int baseSector, int*count){
    //printf("In inode %d find link count for %d\n", inodeNum, inodeNum_target);
    struct ext2_inode inode;
    read_inode(superBlock, baseSector, inodeNum, &inode);
    int numBlocks = inode.i_size/block_size_bytes;
    if(inode.i_size%block_size_bytes){
      numBlocks++;
    }
    //printf("inode record size: %d\nnum blocks: %d\n", inode.i_size, numBlocks);
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode.i_block, numBlocks, bigBufferOfBlocks);
    // this is the first entry 
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size = 0;
    int n = 0;
    while(size < inode.i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks + size;
        size += entry->rec_len;
        n ++;
        if( entry->inode == inodeNum_target){
           (*count)++;
        }
       if ( n > 2 && entry->file_type == EXT2_FT_DIR){
            find_link_count(superBlock, entry->inode, inodeNum_target, baseSector, count);
       }
    }
}

/*
    get inode number for the target file/dir
    if not exist, return -1;
 */
int getInodeNumBasedOnPath(struct ext2_inode* inode, unsigned int baseSector, char* path){

    int numBlocks = inode->i_size/block_size_bytes;

    if(inode->i_size%block_size_bytes){
      numBlocks++;
    } 
    //printf("inode record size: %d\nnum blocks: %d\n", inode.i_size, numBlocks);
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode->i_block, numBlocks, bigBufferOfBlocks);
    // this is the first entry 
    struct ext2_dir_entry_2* entry  = (struct ext2_dir_entry_2*)bigBufferOfBlocks;
    int size;
    size = 0;
    while(size < inode->i_size && entry->inode != 0){
        entry = (void*)bigBufferOfBlocks + size;
        size += entry->rec_len;
        char file_name[EXT2_NAME_LEN +1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        if(strncmp(file_name, path, strlen(path)) == 0){
            printf("Found it ! %10u %s, type: %d\n", entry->inode, file_name, entry->file_type);
            return entry->inode;
        }
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

/*
 *  check all of teh directories of this file system
 */
void check_all_directory(struct ext2_super_block* superBlock, unsigned baseSector){
    printf("-------------Start PASS ONE checking directories------------\n\n");    
    checkDirectory(superBlock, 2, baseSector);
    printf("-------------End PASS ONE-------------\n\n");
}

/*
 *    add inode as lost file into lost and found dir 
 */

void addInodeAsFileToDir(struct ext2_super_block* superBlock, 
    unsigned int baseSector, struct ext2_inode* inode_dir, int inodeNum, int file_type){
    // here we temporarily assume the number of blocks is not more than 12
    int numBlocks = inode_dir->i_size/block_size_bytes;
    if(inode_dir->i_size%block_size_bytes){
            numBlocks ++;
    }
    unsigned char bigBufferOfBlocks[numBlocks * block_size_bytes];
    fetch_all_blocks(baseSector, inode_dir->i_block, numBlocks, bigBufferOfBlocks);
    int size = 0;
    struct ext2_dir_entry_2* prev;
    struct ext2_dir_entry_2*  entry = (struct ext2_dir_entry_2 *)bigBufferOfBlocks;
    
    // the first entry point to itself
    int thisInodeNum = entry->inode;

    while( size < inode_dir->i_size){
        entry = (void*)bigBufferOfBlocks + size;
        if(entry->inode == 0){
            // haha we find a place
            printf("we have a place to insert the inode num at blockIndex %d, at offset %d\n", size/block_size_bytes, size%block_size_bytes);
            entry->inode = inodeNum;
            entry->file_type = file_type;
            sprintf(entry->name, "%d", inodeNum);
            entry->name_len = strlen(entry->name);
            entry->rec_len  = block_size_bytes - size;
            prev->rec_len = EXT2_DIR_REC_LEN(prev->name_len);
            // need to write block back to the disk
            int blockIndex = size/block_size_bytes;
            unsigned int blockId = inode_dir->i_block[blockIndex];
            printf("block id %d, sector at %d\n", blockId, baseSector + blockId * 2);
            write_sectors(baseSector + blockId * 2, 2, bigBufferOfBlocks);
            
            // if file_type == 2, we need to check PASS 1 correctness
            checkDirectory(superBlock, inodeNum, baseSector);

           // need to write the inode back to the disk 
            inode_dir->i_links_count += 1;
            int secNum = inodeToSector(superBlock, baseSector, thisInodeNum);
            unsigned char inodeSector[sector_size__bytes];
            read_sectors(secNum, 1, inodeSector); 
            int secOffset = (thisInodeNum - 1) % 4;
            memcpy(inodeSector + secOffset*sizeof(struct ext2_inode), inode_dir, sizeof(struct ext2_inode));
            write_sectors(secNum, 1, inodeSector);
            break;
        }
        size += EXT2_DIR_REC_LEN(entry->name_len);
        prev = entry;
    }
}


/*
 *  check unreferenced inode
 */
void check_unreference_count(struct ext2_super_block* superBlock, unsigned baseSector){
    printf("-------------Start PASS TWO checking unreferenced count---------------\n\n");
    int totalNumInodes = superBlock->s_inodes_count;
    int inodeNum;
    for(inodeNum = 2; inodeNum <= totalNumInodes; inodeNum++){
            int count = 0;
            struct ext2_inode testInode;
            read_inode(superBlock, baseSector, inodeNum, &testInode);
            int storedLinkCount = testInode.i_links_count;
            find_link_count(superBlock, 2, inodeNum,  baseSector, &count);
            if( count == 0 && storedLinkCount != 0){
                printf("NOTE: inode %d is not referenced by anyone\n", inodeNum);
                char lostFound[] = "/lost+found"; 
                struct ext2_inode thisInode;
                int file_type = 1;
                findInodeBasedOnPath(superBlock, baseSector, lostFound, &thisInode);
                if( S_ISDIR(testInode.i_mode)){
                    file_type = 2;
              }
                addInodeAsFileToDir(superBlock, baseSector, &thisInode, inodeNum, file_type);
                
                //print_directory(superBlock, &thisInode, baseSector);
            }
           /* else if( count != 0 && storedLinkCount != count){
                printf("NOTE: inode %d stored link count: %d, the actual link count: %d\n", 
                                                            inodeNum, storedLinkCount, count);
                testInode.i_links_count = count;
                int secNum = inodeToSector(superBlock, baseSector, inodeNum);
                unsigned char inodeSector[sector_size__bytes];
                read_sectors(secNum, 1, inodeSector); 
                int secOffset = (inodeNum - 1) % 4;
                memcpy(inodeSector + secOffset*sizeof(struct ext2_inode), &testInode, sizeof(struct ext2_inode));
                write_sectors(secNum, 1, inodeSector);
            }*/
    }
    printf("-------------End PASS TWO-------------\n\n");
}




/*
 *  check reference count
 */
 void check_referenced_count(struct ext2_super_block* superBlock, unsigned baseSector){
      printf("-------------Start PASS THREE checking reference count---------------\n\n");
    int totalNumInodes = superBlock->s_inodes_count;
    int inodeNum;
    for(inodeNum = 2; inodeNum <= totalNumInodes; inodeNum++){
            int count = 0;
            struct ext2_inode testInode;
            read_inode(superBlock, baseSector, inodeNum,&testInode);
            int storedLinkCount = testInode.i_links_count;
            find_link_count(superBlock, 2, inodeNum,  baseSector, &count);
            if( count != 0 && storedLinkCount != count){
                printf("NOTE: inode %d stored link count: %d, the actual link count: %d\n", 
                                                            inodeNum, storedLinkCount, count);
                testInode.i_links_count = count;
                int secNum = inodeToSector(superBlock, baseSector, inodeNum);
                unsigned char inodeSector[sector_size__bytes];
                read_sectors(secNum, 1, inodeSector); 
                int secOffset = (inodeNum - 1) % 4;
                memcpy(inodeSector + secOffset*sizeof(struct ext2_inode), &testInode, sizeof(struct ext2_inode));
                write_sectors(secNum, 1, inodeSector);
            }
          }
      printf("-------------End PASS THREE-------------\n\n");
 }




void part2Test(int partition){
    printf("____________________Below is for tesing purpose___________________\n");
    unsigned char MBR[sector_size__bytes];
    read_sectors(0, 1, MBR);
    print_partition_table(MBR);
    int64_t baseSector = print_partition(MBR, partition);
    struct ext2_super_block superBlock;
    
    read_superBlock(baseSector, &superBlock);
    print_superBlock(&superBlock);
    print_all_directory(&superBlock, baseSector);
    
    int counter = 0;
    int inodeNum;
    int numBlocks = 0;
    int numInode = superBlock.s_inodes_count;
   for(inodeNum = 1 ; inodeNum <= numInode; inodeNum ++ ){
       struct ext2_inode testInode;
       read_inode(&superBlock,baseSector, inodeNum, &testInode);
       if( isInodeInBitMap(&superBlock, inodeNum, baseSector)){
           counter++;
           numBlocks += testInode.i_blocks/(2<<superBlock.s_log_block_size);
        }
    }
    printf("total Inodes: %d, actual count: %d, total blocks: %d, actual count: %d\n", superBlock.s_inodes_count - superBlock.s_free_inodes_count, counter,superBlock.s_blocks_count -superBlock.s_free_blocks_count, numBlocks );
    
    int blockId;
    int counter2 = 0;
    for(blockId = 1; blockId <= superBlock.s_blocks_count; blockId++){
        if( isBlockInBitMap(&superBlock, blockId, baseSector)){
            counter2++;
        if((blockId - 1) / 8192 == 2){
       //     printf("BLOCK %d\n", blockId);
        }
        }
    }
    printf("actual count of blocks %d\n", counter2);


}

void smallTest(){
    printf("------------start small test------------\n");
    unsigned char tmp[block_size_bytes];
    read_sectors(63 + 256 * 2, 2, tmp);
}

int
main (int argc, char **argv)
{
	unsigned char MBR[sector_size__bytes];	/* temporary buffer */
	int           partitionToRead = -1;			/* IN: partition to read */
	int           partitionToFix = -1;           /* IN: partition to fix*/
    char* 		  disk;
	char 		  op;
	while((op = getopt(argc, argv, "i:p:f:")) != EOF){
		switch(op){
			case 'i':
				disk = optarg;
                break;
			case 'p':
				partitionToRead = atoi(optarg);
				break;
            case 'f':
                partitionToFix = atoi(optarg);
                break;
        }
	}
	if ((device = open(disk, O_RDWR)) == -1) {
	 perror("Could not open device file");
	 exit(-1);
	}   
    //part2Test();
    //smallTest();
    read_sectors(0, 1, MBR);
    if(partitionToRead >= 0){
        print_partition(MBR, partitionToRead);
        //part2Test(partitionToRead);
  }
    if(partitionToFix >0){
        int64_t baseSector = print_partition(MBR, partitionToFix);
        struct ext2_super_block superBlock;
        read_superBlock(baseSector, &superBlock);
        check_all_directory(&superBlock ,baseSector);
        check_unreference_count(&superBlock, baseSector);
        check_referenced_count(&superBlock, baseSector);
        check_all_blocks(&superBlock, baseSector);
    }else if(partitionToFix == 0){
        for(partitionToFix = 1; partitionToFix <=6; partitionToFix ++){
            
        int64_t baseSector = print_partition(MBR, partitionToFix);
        if(baseSector != 0 && partitionToFix != 4){
            struct ext2_super_block superBlock;
            read_superBlock(baseSector, &superBlock);
            check_all_directory(&superBlock ,baseSector);
            check_unreference_count(&superBlock, baseSector);
            check_referenced_count(&superBlock, baseSector);
            check_all_blocks(&superBlock, baseSector);
             }
        }
    }

    close(device);
	return 0;
}

/* EOF */
