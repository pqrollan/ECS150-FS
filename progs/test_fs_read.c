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

void simple_read()
{
        fs_mount("disk1.fs");

        int file = fs_open("hello.txt");
        fs_stat(file);
        assert(file != -1);

        char buffer[5];
        memset(buffer, 0, 5);

        assert(fs_read(file, buffer, 3) == 3);
        
        printf("%s\n", buffer);
        fs_read(file, buffer, 4);
        printf("%s\n", buffer);

        fs_close(file);

        fs_umount();

}

void simple_read_to_end()
{
        fs_mount("disk1.fs");

        int file = fs_open("hello.txt");

        char buffer[20];
        memset(buffer, 0, 20);

        assert(20 == fs_read(file, buffer, 20));
        
        printf("%s\n", buffer);

        fs_close(file);

        fs_umount();

}

void long_read()
{
        fs_mount("disk1.fs");

        int file = fs_open("longfile.txt");
        fs_stat(file);

        char buffer[fs_stat(file)+1];
        memset(buffer, 0, fs_stat(file)+1);

        assert(fs_read(file, buffer, fs_stat(file)) == fs_stat(file));
        
        printf("%s\n", buffer);

        fs_close(file);

        fs_umount();
}

void long_read_offset()
{
        fs_mount("disk1.fs");

        int file = fs_open("longfile.txt");
        fs_stat(file);

        char buffer[21];
        memset(buffer, 0, 21);

        fs_lseek(file, 4097);

        assert(fs_read(file, buffer, 20) == 20);
        
        printf("%s\n", buffer);

        fs_close(file);

        fs_umount();       
}

int main(int argc, char **argv)
{
        simple_read();
        simple_read_to_end();
        long_read();
        long_read_offset();
	return 0;
}
