cd ..
cd code
cd build.linux
make clean
make depend
make
cd ..
cd test
make clean
make halt fileIO_test1 fileIO_test2
../build.linux/nachos -e fileIO_test1
cat file1.test
echo ""
../build.linux/nachos -e fileIO_test2