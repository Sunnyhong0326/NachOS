make clean
make
../build.linux/nachos -f
../build.linux/nachos -mkdir /test1
../build.linux/nachos -mkdir /test2
../build.linux/nachos -cp FS_test1 /FS_test1
../build.linux/nachos -lr /
../build.linux/nachos -e /FS_test1
../build.linux/nachos -lr /
../build.linux/nachos -p /test1/file1
../build.linux/nachos -cp FS_test2 /FS_test2
../build.linux/nachos -e /FS_test2
../build.linux/nachos -lr /
../build.linux/nachos -lr /
../build.linux/nachos -mkdir /test1/hw1
../build.linux/nachos -mkdir /test1/hw2
../build.linux/nachos -lr /
../build.linux/nachos -cp FS_test1 /test1/hw1/FS_test1
../build.linux/nachos -cp FS_test2 /test1/hw1/FS_test2
../build.linux/nachos -lr /
../build.linux/nachos -r /FS_test1
../build.linux/nachos -lr /
../build.linux/nachos -r /FS_test2
../build.linux/nachos -lr /
../build.linux/nachos -rr /test1
../build.linux/nachos -lr /
../build.linux/nachos -l /