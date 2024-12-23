./build-inclass-tests.sh
./build-riscv-elfs.sh
pushd build
make -j64
popd

pushd test/riscv-elf
for file in *.riscv; do
    echo "testing $file"
    Simulator $file
done
popd

pushd test-without-syscall
for file in *.riscv; do
    echo "testing $file"
    Simulator $file
done
popd