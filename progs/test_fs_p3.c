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

void simple_open()
{
        fs_mount("disk.fs");

        fs_create("hello.txt");

        int filed = fs_open("hello.txt");

        print_fdarray();

        fs_close(filed);

        print_fdarray();

        fs_delete("hello.txt");

        fs_umount();       
}

void multi_open()
{
        fs_mount("disk.fs");

        fs_create("hello.txt");
        fs_create("hello1.txt");
        fs_create("hello2.txt");

        int file1 = fs_open("hello.txt");
        int file2 = fs_open("hello1.txt");
        int file3 = fs_open("hello2.txt");

        print_fdarray();

        fs_close(file1);
        fs_close(file2);
        fs_close(file3);

        print_fdarray();

        fs_delete("hello.txt");
        fs_delete("hello1.txt");
        fs_delete("hello2.txt");

        fs_ls();

        fs_umount();
}

void lseek_stat()
{
        fs_mount("disk1.fs");

        int file = fs_open("hello.txt");
        printf("%d\n", fs_stat(file));

        print_fdarray();
        
        fs_lseek(file, 5);

        print_fdarray();

        fs_umount();

}


int main(int argc, char **argv)
{
        simple_open();
        multi_open();
        lseek_stat();

	return 0;
}
