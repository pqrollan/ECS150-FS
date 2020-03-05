#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fs.h>
#include <fs1.h>

void simple_write()
{
        fs_mount("disk1.fs");

        int file = fs_open("hello.txt");
        fs_stat(file);
        assert(file != -1);

        char buffer[5];
        memset(buffer, 49, 5);

        assert(fs_write(file, buffer, 3) == 3);
       
        fs_lseek(file, 0);
        memset(buffer, 0, 5);
        fs_read(file, buffer, 3);

        printf("%s\n", buffer);
        fs_read(file, buffer, 4);
        printf("%s\n", buffer);

        fs_close(file);

        fs_umount();

}

void simple_append()
{
        fs_mount("disk1.fs");

        int file = fs_open("hello.txt");
        fs_stat(file);
        assert(file != -1);

        char buffer[5];
        memset(buffer, 49, 5);

        fs_lseek(file, fs_stat(file));
        assert(fs_write(file, buffer, 3) == 3);
        
        fs_lseek(file, 0);
        char buffer1[51];
        memset(buffer1, 0, 51);
        fs_read(file, buffer1, 50);

        printf("%s\n", buffer1);

        fs_close(file);

        fs_umount();

}

void long_write()
{
        fs_mount("disk1.fs");

        int file = fs_open("longfile.txt");
        printf("size of file, start: %d\n", fs_stat(file));

        char buffer[fs_stat(file)+30];
        memset(buffer, 49, fs_stat(file)+30);

        fs_lseek(file, 0);
        fs_write(file, buffer, fs_stat(file)+30);
        
        printf("size of file, end: %d\n", fs_stat(file));

        fs_lseek(file, 0);
        char buffer1[fs_stat(file)+1];
        memset(buffer1, 0, fs_stat(file)+1);
        fs_read(file, buffer1, fs_stat(file)+1);

        printf("%s\n", buffer1);


        fs_close(file);

        fs_umount();
}

void long_write_offset()
{
        fs_mount("disk1.fs");

        fs_info();

        int file = fs_open("longfile.txt");
        printf("\n Long offset test\n");
        printf("size of file, start: %d\n", fs_stat(file));

        char buffer[10000];
        memset(buffer, 50, 10000);

        fs_lseek(file, fs_stat(file));

        assert(fs_write(file, buffer, 10000) == 10000);
        
        fs_lseek(file, 0);
        char buffer1[fs_stat(file)+1];
        memset(buffer1, 0, fs_stat(file)+1);
        fs_read(file, buffer1, fs_stat(file)+1);

        printf("\n \n \n %s\n", buffer1);
        printf("%d\n", fs_stat(file));
        fs_close(file);

        fs_umount();       
}

int main(int argc, char **argv)
{
        simple_write();
        simple_append();
        long_write();
        long_write_offset();
	return 0;
}
