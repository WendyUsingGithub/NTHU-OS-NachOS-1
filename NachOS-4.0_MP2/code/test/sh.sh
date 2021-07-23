cd ..
cd code
cd build.linux
make clean
make depend
make
cd ..
cd test
make clean
make consoleIO_test1 consoleIO_test2
../build.linux/nachos -e consoleIO_test1 -e consoleIO_test2