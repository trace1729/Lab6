# RISC-V Simulator

A Simple RISC-V CPU Simulator with 5 Stage Pipeline, bubble/stall version

## Features

1. Supporting basic RISC-V instructions from the RV64I instruction set.
2. Simulation of five stage pipeline like that in the "Computer Organization and Design, Hardware/Software Interface" Book.

## Compile

```
mkdir build
cd build
cmake ..
make
```

## Usage

```
./Simulator riscv-elf-file-name [-v] [-s]
```
Parameters:

1. `-v` for verbose output, can redirect output to file for further analysis
2. `-s` for single step execution, often used in combination with `-v`.

eg:  
```
./Simulator -v ../test-without-syscall/add.riscv
```

## core file
main to run: src/MainCPU.cpp  
simulator: src/Simulator.cpp  
memory: src/MemoryManager.cpp


## Compile your own .riscv
To set up the compilation, execution, and testing environment related to RISC-V, especially focusing on the RISC-V 64I instruction set, referring to the RISC-V Specification 2.2, the following steps were taken for configuring the environment:

1. Downloaded riscv-tools from GitHub and configured, compiled, and installed riscv-gnu-toolchain specifically for the Linux platform.
2. Downloaded, compiled, and installed riscv-qemu from GitHub to use the official emulator as a reference.

It is crucial to note that when compiling riscv-gnu-toolchain, it is necessary to specify that the toolchain and the C standard library use the RV64I instruction set. Otherwise, during compilation, the compiler might utilize extensions like RV64C, RV64D, among others. Even if the compiler is set to utilize only the RV64I instruction set, it may still link standard library functions that use extension instruction sets. Therefore, to obtain an ELF program that solely uses the RV64I standard instruction set, it is essential to recompile riscv-gnu-toolchain with the following options:

```
mkdir build; cd build
../configure --with-arch=rv64i --prefix=/path/to/riscv64i
make -j$(nproc)
```

During compilation, using -march=rv64i instructs the compiler to generate an ELF program targeting the RV64I standard instruction set:

```
riscv64-unknown-elf-gcc -march=rv64i test/arithmetic.c test/lib.c -o riscv-elf/arithmetic.riscv
```

ps: when write your own .c, please do not use syscall since not supported currently.