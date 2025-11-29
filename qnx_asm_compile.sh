#!/bin/bash -x

cd /home/vpodzime/sources/mender/mender/build/_deps/boost-build/libs/context
for asm_source in make_arm64_aapcs_elf_gas.S jump_arm64_aapcs_elf_gas.S ontop_arm64_aapcs_elf_gas.S; do
  ntoaarch64-as -o CMakeFiles/boost_context.dir/src/asm/$asm_source.o -c /home/vpodzime/sources/mender/mender/build/_deps/boost-src/libs/context/src/asm/$asm_source
done
