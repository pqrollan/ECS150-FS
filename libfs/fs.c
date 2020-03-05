#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/** Maximum filename length (including the NULL character) */
//#define FS_FILENAME_LEN 16

/** Maximum number of files in the root directory */
//#define FS_FILE_MAX_COUNT 128

/** Maximum number of open files */
//#define FS_OPEN_MAX_COUNT 32

#define BLOCK_SIZE 4096
#define FAT_EOC 0xFFFF

struct __attribute__ ((__packed__)) SuperBlock {
        char signature[8];
        uint16_t blockcount;
        uint16_t rootdirindex;
        uint16_t datablockindex;
        uint16_t datablockcount;
        uint8_t fatblocks;
        uint8_t padding[4079];
};

struct __attribute__ ((__packed__)) FEntry {
        char filename[16];
        uint32_t size;
        uint16_t index;
        uint8_t padding[10];
};

struct fileDescriptor{
        char filename[16];
        int offset;
        uint16_t fileIndex;

};

struct fileDescriptor FDArray[32];
struct SuperBlock superblock;
uint16_t *fat = NULL;
struct FEntry rootdir[128];

int fs_mount(const char *diskname)
{
        if (block_disk_open(diskname)) {
                return -1;
        }

        block_read(0, &superblock);
        
        if (block_disk_count() != superblock.blockcount) {
                return -1;
        }

        if (memcmp(superblock.signature, "ECS150FS", 8)) {
                return -1;
        }



        fat = malloc(superblock.fatblocks * BLOCK_SIZE);
        for (int i = 0; i < superblock.fatblocks; i++) {
	        block_read(1 + i, &fat[(i * BLOCK_SIZE)/2]);
        } 

        block_read(superblock.rootdirindex, rootdir);

        for (int i =0; i< FS_OPEN_MAX_COUNT; i++){
                memset(FDArray[i].filename, 0, FS_FILENAME_LEN);
        }

        return 0;
}

int fs_umount(void)
{
        if (fat == NULL) {
                return -1;
        }

        for (int i = 0; i < superblock.fatblocks; i++) {
                block_write(1 + i, &fat[i * BLOCK_SIZE]);
        }

        free(fat);

        block_write(superblock.rootdirindex, rootdir);

        if (block_disk_close()) {
                return -1;
        }

        return 0;
}

int fs_info(void)
{
        if (fat == NULL) {
                return -1;
        }

        int rdir_free = 0;
        int fat_free = 0;
        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (rootdir[i].filename[0] == 0) {
                        rdir_free++;
                }
        }
        for (int i = 0; i < superblock.datablockcount; i++) {
                if (fat[i] == 0) {
                        fat_free++;
                }
        }

        printf("FS Info:\n");
        printf("total_blk_count=%u\n", superblock.blockcount);
        printf("fat_blk_count=%u\n", superblock.fatblocks);
        printf("rdir_blk=%u\n", superblock.rootdirindex);
        printf("data_blk=%u\n", superblock.datablockindex);
        printf("data_blk_count=%u\n", superblock.datablockcount);
        printf("fat_free_ratio=%d/%u\n", fat_free, superblock.datablockcount);
        printf("rdir_free_ratio=%d/%d\n", rdir_free, FS_FILE_MAX_COUNT);
        return 0;
}

int fs_create(const char *filename)
{
        if (filename == NULL) {
                return -1;
        }

        int filenamelen = strlen(filename);
        if (filenamelen > FS_FILENAME_LEN || filenamelen < 1) {
                return -1;
        }
        
        for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if(!strcmp(filename, rootdir[i].filename)) {
                        return -1;
                }
        }

        int index = -1;
        for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (rootdir[i].filename[0] == 0) {
                        index = i;
                        break;
                }
        }
        if (index == -1) {
               return -1; 
        }

        strcpy(rootdir[index].filename, filename);
        rootdir[index].size = 0;

        int fatindex = -1;
        for (int i = 1; i < superblock.datablockcount; i++) {
                if (fat[i] == 0) {
                        fatindex = i;
                        fat[fatindex] = FAT_EOC;
                        break;
                }
        }
        if (fatindex == -1) {
                return -1;
        }

        rootdir[index].index = fatindex;
        return 0;
}

int fs_delete(const char *filename)
{
        if (filename == NULL || strlen(filename) < 1) {
                return -1;
        }

        int index = -1;
        for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (!strcmp(filename, rootdir[i].filename)) {
                        index = i;
                        break;
                }
        }

        if (index == -1) {
               return -1; 
        }

        int fatindex = rootdir[index].index;
        while (fat[fatindex] != FAT_EOC) {
                int temp = fatindex;
                fatindex = fat[fatindex];
                fat[temp] = 0;
        }
        fat[fatindex] = 0;
        
        rootdir[index].index = 0;
        rootdir[index].size = 0;
        memset(rootdir[index].filename, 0, FS_FILENAME_LEN);

        return 0;
}

int fs_ls(void)
{
        if (rootdir == NULL) {
               return -1; 
        }
        printf("FS Ls:\n");

        for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (rootdir[i].filename[0] == 0) {
                        continue;
                }
                printf("file: %s, size: %d, data_blk: %d\n", rootdir[i].filename,
                        rootdir[i].size, rootdir[i].index);
        }

        return 0;
}

int fs_open(const char *filename)
{
        if (filename == NULL || strlen(filename) < 1) {
                return -1;
        }

        int index = -1;
        for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (!strcmp(filename, rootdir[i].filename)) {
                        index = i;
                        break;
                }
        }
        if (index == -1) {
               return -1; 
        }

        int FDIndex= -1;
        for (int i=0; i<FS_OPEN_MAX_COUNT; i++){
                if (!strcmp(FDArray[i].filename, "")){
                        FDIndex = i;
                        break;
                }
        }
        if (FDIndex == -1){
                return -1;
        }

        strcpy(FDArray[FDIndex].filename, filename);
        FDArray[FDIndex].offset= 0;
        FDArray[FDIndex].fileIndex= index;

        return FDIndex;
}

int fs_close(int fd)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }

        memset(FDArray[fd].filename, 0, FS_FILENAME_LEN);
        FDArray[fd].offset=0;
        FDArray[fd].fileIndex = FS_FILE_MAX_COUNT;
        return 0;
}

/* 
void print_fdarray(void)
{
        for (int i=0; i<FS_OPEN_MAX_COUNT; i++){
                if (strcmp(FDArray[i].filename, "")) {
                        printf("Filename: %s\n", FDArray[i].filename);
                        printf("File Descriptor: %d\n", i);
                        printf("File Offset: %d\n", FDArray[i].offset);
                        printf("Root Directory Index: %d\n",
                        FDArray[i].fileIndex);
                }
        }

}*/

int fs_stat(int fd)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }
        return rootdir[FDArray[fd].fileIndex].size;
}

int fs_lseek(int fd, size_t offset)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }
        if (offset > (size_t) fs_stat(fd)){
                return -1;
        }
        FDArray[fd].offset = offset;

        return 0;
}

int get_datablock(int fd, size_t offset)
{
        int fatindex = rootdir[FDArray[fd].fileIndex].index;

        while (1) {
                if (offset < BLOCK_SIZE) {
                        break;
                } else {
                        fatindex = fat[fatindex];
                        offset -= BLOCK_SIZE;
                }
        }

        return fatindex;
}

size_t min(size_t a, size_t b)
{
        if (a < b) {
                return a;
        } else {
                return b;
        }
}

uint32_t max(uint32_t a, uint32_t b) {
        if (a > b) {
                return a;
        } else {
                return b;
        }
}

void print_fileblocks(int fd) {
        int db = rootdir[FDArray[fd].fileIndex].index;
        do {
                printf("%d -> ", db);
                db = fat[db];
        } while (db != FAT_EOC);

        printf("\n");
}

int alloc_blocks(int fd, size_t offset, size_t size)
{
        int db = get_datablock(fd, offset);
        int alloc_count = 0;
        size_t block_offset = offset % BLOCK_SIZE;
        
        if (size > BLOCK_SIZE - block_offset) {
                size -= BLOCK_SIZE - block_offset;
        } else {
                return alloc_count;
        }
        
        while (fat[db] != FAT_EOC && size > BLOCK_SIZE) {
                db = fat[db];
                size -= BLOCK_SIZE;
        }

        if (fat[db] != FAT_EOC) {
                return alloc_count;
        }

        do {
                int fatindex = -1;
                for (int i = 1; i < superblock.datablockcount; i++) {
                        if (fat[i] == 0) {
                                fatindex = i;
                                break;
                        }
                }
                if (fatindex == -1) {
                        fat[db] = FAT_EOC;
                        return alloc_count;
                }

                fat[db] = fatindex;
                db = fatindex;
                fat[db] = FAT_EOC;
                alloc_count++;
                if (size <= BLOCK_SIZE){
                        break;
                }
                size -= BLOCK_SIZE;
                
        } while(1);
        
        return alloc_count;
}

int fs_write(int fd, void *buf, size_t count)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }
        size_t start_offset = FDArray[fd].offset;
        size_t offset = start_offset;
        int db = get_datablock(fd, offset);
        size_t num_written = 0;
        char page_buffer[BLOCK_SIZE];

        alloc_blocks(fd, offset, count);
        while (num_written < count) {
                size_t block_offset = offset % BLOCK_SIZE;
                size_t num_to_write = min(BLOCK_SIZE - block_offset, count -
                        num_written);

                block_read(db + superblock.datablockindex, page_buffer);
                memcpy(&page_buffer[block_offset], buf + num_written,
                        num_to_write);
                block_write(db + superblock.datablockindex, page_buffer);

                num_written += num_to_write;
                offset += num_to_write;
                if (fat[db] == FAT_EOC) {
                        break;
                }
                db = fat[db];
        }

        FDArray[fd].offset = offset;
        rootdir[FDArray[fd].fileIndex].size = max(start_offset + num_written, 
                rootdir[FDArray[fd].fileIndex].size);
        return num_written;
}

int fs_read(int fd, void *buf, size_t count)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }

        size_t offset = FDArray[fd].offset;
        int db = get_datablock(fd, offset);
        size_t num_read = 0;
        char page_buffer[BLOCK_SIZE];

        while (num_read < count) {
                size_t block_offset = offset % BLOCK_SIZE;
                size_t num_to_read = min(BLOCK_SIZE - block_offset, count - num_read);

                block_read(db + superblock.datablockindex, page_buffer);
                memcpy(buf + num_read, &page_buffer[block_offset],
                        num_to_read);

                num_read += num_to_read;
                offset += num_to_read;
                db = fat[db];
                if (db == FAT_EOC) {
                        break;
                }
        }
        
        FDArray[fd].offset = offset;
        return num_read;
}
