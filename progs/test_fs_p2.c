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

void simple_create()
{
        fs_mount("disk.fs");

        fs_info();

        fs_create("hello.txt");

        fs_ls();

        fs_delete("hello.txt");

        fs_ls();

        fs_umount();       
}

void multi_create()
{
        fs_mount("disk.fs");

        fs_info();

        fs_create("hello.txt");
        fs_create("hello1.txt");
        fs_create("hello2.txt");

        fs_ls();

        fs_delete("hello.txt");
        fs_delete("hello1.txt");
        fs_delete("hello2.txt");

        fs_ls();

        fs_umount();
}


int main(int argc, char **argv)
{
        simple_create();
        multi_create();

	return 0;
}
