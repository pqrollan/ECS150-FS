#!/bin/bash

RETVAL=""
NUM_DISKS=0
REF="ref.txt"
TEST="test.txt"

create_disk() {
        DISKNAME="disk$NUM_DISKS.fs"
        ./fs_make.x $DISKNAME $1
        ((NUM_DISKS++))
        RETVAL=$DISKNAME
}

repeat_num() {
        for num in $(seq 1 $1)
        do
                echo "$2"
        done
}

create_file() {
        repeat_num $2 $3 > $1
}

test_diff() {
        TEST_VAL=3
        if [ $# == 3 ]
        then
        ./fs_ref.x $1 $2 > $REF
        ./test_fs.x $1 $3 > $TEST
        diff $REF $TEST
        else
        ./fs_ref.x $1 $2 $4 > $REF
        ./test_fs.x $1 $3 $4 > $TEST
        diff $REF $TEST
        fi
}

run_both() {
        TEST_VAL=3
        if [ $# == 3 ]
        then
        ./fs_ref.x $1 $2
        ./test_fs.x $1 $3
        else
        ./fs_ref.x $1 $2 $4
        ./test_fs.x $1 $3 $4
        fi
}

basic_test() {
        create_file hello.txt 1 20
        create_disk 5
        DISK1=$RETVAL
        create_disk 5
        DISK2=$RETVAL

        run_both add $DISK1 $DISK2 hello.txt
        test_diff cat $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2
        test_diff stat $DISK1 $DISK2 hello.txt
        run_both rm $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2
        

        rm hello.txt
}

remake_test() {
        create_file hello.txt 1 20
        create_disk 5
        DISK1=$RETVAL
        create_disk 5
        DISK2=$RETVAL

        run_both add $DISK1 $DISK2 hello.txt
        test_diff cat $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2
        test_diff stat $DISK1 $DISK2 hello.txt
        run_both rm $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2
        
        run_both add $DISK1 $DISK2 hello.txt
        test_diff cat $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2
        test_diff stat $DISK1 $DISK2 hello.txt
        run_both rm $DISK1 $DISK2 hello.txt
        test_diff ls $DISK1 $DISK2

        rm hello.txt
}

expensive_test() {
        create_file hello.txt 20 1
        create_file oneblk.txt 4096 2
        create_file twoblk.txt 8192 3
        create_file threeblk.txt 10000 4
        create_disk 100
        DISK1=$RETVAL
        create_disk 100
        DISK2=$RETVAL
        FILENAMES=( "hello.txt" "oneblk.txt" "twoblk.txt" "threeblk.txt" )

        for var in "${FILENAMES[@]}";do
                run_both add $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do
                test_diff cat $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2

        for var in "${FILENAMES[@]}";do
                test_diff stat $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do       
                run_both rm $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2
        

        rm hello.txt
}

so_many_test() {
        create_file hello.txt 20 1
        create_file oneblk.txt 4096 2
        create_file twoblk.txt 8192 3
        create_file threeblk.txt 10000 4
        create_file thirtyblk.txt 122880 5
        create_disk 100
        DISK1=$RETVAL
        create_disk 100
        DISK2=$RETVAL
        FILENAMES=( "hello.txt" "oneblk.txt" "twoblk.txt" "threeblk.txt" \
                "thirtyblk.txt")

        for var in "${FILENAMES[@]}";do
                run_both add $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do
                test_diff cat $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2

        for var in "${FILENAMES[@]}";do
                test_diff stat $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do       
                run_both rm $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2
        

        rm *.txt
}

offset_write_test() {
        create_file hello.txt 8192 1
        create_file add.txt 1000 2
        create_disk 20
        DISK1=$RETVAL
        create_disk 20
        DISK2=$RETVAL
        FILENAMES=( "hello.txt" "add.txt")

        for var in "${FILENAMES[@]}";do
                run_both add $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do
                test_diff cat $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2

        for var in "${FILENAMES[@]}";do
                test_diff stat $DISK1 $DISK2 $var
        done

        ./test_fs_more.x write $DISK2 "hello.txt" "add.txt" "4000"
        cat add.txt | dd of=hello.txt seek=4000 bs=1 conv=notrunc
        ./fs_ref.x rm $DISK1 "hello.txt"
        ./fs_ref.x add $DISK1 "hello.txt"
        
        test_diff cat $DISK1 $DISK2 "hello.txt"

        for var in "${FILENAMES[@]}";do       
                run_both rm $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2
        

        rm *.txt
}

offset_newblk_test() {
        create_file hello.txt 4096 1
        create_file add.txt 4096 2
        create_disk 20
        DISK1=$RETVAL
        create_disk 20
        DISK2=$RETVAL
        FILENAMES=( "hello.txt" "add.txt")

        for var in "${FILENAMES[@]}";do
                run_both add $DISK1 $DISK2 $var
        done
        
        for var in "${FILENAMES[@]}";do
                test_diff cat $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2

        for var in "${FILENAMES[@]}";do
                test_diff stat $DISK1 $DISK2 $var
        done

        ./test_fs_more.x write $DISK2 "hello.txt" "add.txt" "4000"
        cat add.txt | dd of=hello.txt seek=4000 bs=1 conv=notrunc
        ./fs_ref.x rm $DISK1 "hello.txt"
        ./fs_ref.x add $DISK1 "hello.txt"
        
        test_diff cat $DISK1 $DISK2 "hello.txt"

        for var in "${FILENAMES[@]}";do       
                run_both rm $DISK1 $DISK2 $var
        done

        test_diff ls $DISK1 $DISK2
        

        rm *.txt
}

#
# Run tests
#
run_tests() {
        basic_test
        remake_test
        expensive_test
        so_many_test
        offset_write_test
        offset_newblk_test
}

make
run_tests
make clean
rm *.fs
