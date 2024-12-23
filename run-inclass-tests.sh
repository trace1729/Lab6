mkdir build
cd build
cmake ..
make
./Simulator ../test-without-syscall/add.riscv
./Simulator ../test-without-syscall/mul-div.riscv
./Simulator ../test-without-syscall/n!.riscv
./Simulator ../test-without-syscall/qsort.riscv
./Simulator ../test-without-syscall/simple-function.riscv