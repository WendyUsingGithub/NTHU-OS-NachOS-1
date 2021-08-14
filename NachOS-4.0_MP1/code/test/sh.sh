cd ..
cd code
cd build.linux
make clean
make depend
make
cd ..
cd test
make clean
make fileIO_test1
../build.linux/nachos -e fileIO_test1
cat file1.test
echo ""