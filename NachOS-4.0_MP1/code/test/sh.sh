cd ..
cd code
cd build.linux
make clean
make depend
make
cd ..
cd test
make clean
make add
../build.linux/nachos -e add
# make consoleIO_test1
# ../build.linux/nachos -e consoleIO_test1
# ../build.linux/nachos -e consoleIO_test1a