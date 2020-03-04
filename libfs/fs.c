#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
#include "fs1.h"

/** Maximum filename length (including the NULL character) */
//#define FS_FILENAME_LEN 16

/** Maximum number of files in the root directory */
//#define FS_FILE_MAX_COUNT 128

/** Maximum number of open files */
//#define FS_OPEN_MAX_COUNT 32

#define BLOCK_SIZE 4096
#define FAT_EOC 0xFFFF

/* TODO: Phase 1 */
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

/**
 * fs_mount - Mount a file system
 * @diskname: Name of the virtual disk file
 *
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. A file system needs to be mounted before files can be read from it
 * with fs_read() or written to it with fs_write().
 *
 * Return: -1 if virtual disk file @diskname cannot be opened, or if no valid
 * file system can be located. 0 otherwise.
 */
int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
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

/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.
 */
int fs_umount(void)
{
	/* TODO: Phase 1 */
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

/**
 * fs_info - Display information about file system
 *
 * Display some information about the currently mounted file system.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_info(void)
{
	/* TODO: Phase 1 */
        if (fat == NULL) {
                return -1;
        }

        printf("Block Count: %u\n", superblock.blockcount);
        printf("Root Directory Index: %u\n", superblock.rootdirindex);
        printf("Data Block Index: %u\n", superblock.datablockindex);
        printf("Data Block Count: %u\n", superblock.datablockcount);
        printf("Fat Block Count: %u\n", superblock.fatblocks);

        return 0;
}

/** TODO: Phase 2
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if @filename is invalid, if a file named @filename already exists,
 * or if string @filename is too long, or if the root directory already contains
 * %FS_FILE_MAX_COUNT files. 0 otherwise.
 */
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

/** TODO: Phase 2
 * fs_delete - Delete a file
 * @filename: File name
 *
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 *
 * Return: -1 if @filename is invalid, if there is no file named @filename to
 * delete, or if file @filename is currently open. 0 otherwise.
 */
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

/** TODO: Phase 2
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_ls(void)
{
        if (rootdir == NULL) {
               return -1; 
        }

        for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if (rootdir[i].filename[0] == 0) {
                        continue;
                }
                printf("%s- size:%d, FAT index:%d\n", rootdir[i].filename,
                        rootdir[i].size, rootdir[i].index);
        }

        return 0;
}

/** TODO: Phase 3
 * fs_open - Open a file
 * @filename: File name
 *
 * Open file named @filename for reading and writing, and return the
 * corresponding file descriptor. The file descriptor is a non-negative integer
 * that is used subsequently to access the contents of the file. The file offset
 * of the file descriptor is set to 0 initially (beginning of the file). If the
 * same file is opened multiple files, fs_open() must return distinct file
 * descriptors. A maximum of %FS_OPEN_MAX_COUNT files can be open
 * simultaneously.
 *
 * Return: -1 if @filename is invalid, there is no file named @filename to open,
 * or if there are already %FS_OPEN_MAX_COUNT files currently open. Otherwise,
 * return the file descriptor.
 */
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

/** TODO: Phase 3
 * fs_close - Close a file
 * @fd: File descriptor
 *
 * Close file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). 0 otherwise.
 */
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


/* REMOVE BEFORE SUBMISSION? */
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

}

/** TODO: Phase 3
 * fs_stat - Get file status
 * @fd: File descriptor
 *
 * Get the current size of the file pointed by file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the current size of file.
 */
int fs_stat(int fd)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }

        return rootdir[FDArray[fd].fileIndex].size;
}

/** TODO: Phase 3
 * fs_lseek - Set file offset
 * @fd: File descriptor
 * @offset: File offset
 *
 * Set the file offset (used for read and write operations) associated with file
 * descriptor @fd to the argument @offset. To append to a file, one can call
 * fs_lseek(fd, fs_stat(fd));
 *
 * Return: -1 if file descriptor @fd is invalid (i.e., out of bounds, or not
 * currently open), or if @offset is larger than the current file size. 0
 * otherwise.
 */
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

int alloc_blocks(int fd, size_t offset, size_t size)
{
        int db = get_datablock(fd, offset);
        int alloc_count = 0;
        size_t block_offset = offset % BLOCK_SIZE;
        
        if (size > BLOCK_SIZE - block_offset) {
                printf("Going to write in multiple blocks\n");
                size -= BLOCK_SIZE - block_offset;
        } else {
                return alloc_count;
        }
        
        while (fat[db] != FAT_EOC && size > BLOCK_SIZE) {
                printf("Traversing used fat\n");
                db = fat[db];
                size -= BLOCK_SIZE;
        }
        printf("Traversed all pre-used fat\n");
        printf("FAT[db]= %d\n", fat[db]);

        /*
        if (size < BLOCK_SIZE) {
                printf("Size before new blocks: %ld\n", size);
                return alloc_count;
        }
        */
        
        
        do {
                printf("allocating new block\n");
                int fatindex = -1;
                for (int i = 1; i < superblock.datablockcount; i++) {
                        if (fat[i] == 0) {
                                fatindex = i;
                                printf("found a spot for next block\n");
                                break;
                        }
                }
        
                if (fatindex == -1) {
                        printf("did not find a spot for next block\n");
                        fat[db] = FAT_EOC;
                        return alloc_count;
                }

                fat[db] = fatindex;
                db = fatindex;
                alloc_count++;
                if (size < 2*BLOCK_SIZE){
                        break;
                }
                size -= BLOCK_SIZE;
                
        } while(1);

        fat[db] = FAT_EOC;

        return alloc_count;
}

/** TODO: Phase 4
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually written.
 */
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
                db = fat[db];
                if (db == FAT_EOC) {
                        printf("breaking on empty FAT\n");
                        break;
                }
        }
        
        FDArray[fd].offset = offset;
        rootdir[FDArray[fd].fileIndex].size = max(start_offset + num_written, 
                rootdir[FDArray[fd].fileIndex].size);
        printf("Num_written: %ld\n", num_written);
        return num_written;
}

/** TODO: Phase 4
 * fs_read - Read from a file
 * @fd: File descriptor
 * @buf: Data buffer to be filled with data
 * @count: Number of bytes of data to be read
 *
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 *
 * The number of bytes read can be smaller than @count if there are less than
 * @count bytes until the end of the file (it can even be 0 if the file offset
 * is at the end of the file). The file offset of the file descriptor is
 * implicitly incremented by the number of bytes that were actually read.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually read.
 */
int fs_read(int fd, void *buf, size_t count)
{
        if (fd<0 || fd>31 || !strcmp(FDArray[fd].filename, "")){
                return -1;
        }

        size_t offset = FDArray[fd].offset;
        int db = get_datablock(fd, offset);
        size_t num_read = 0;
        char page_buffer[BLOCK_SIZE];

        //printf("%ld\n%d\n%ld\n", offset, db, num_read);

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
