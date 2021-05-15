# C Tests
To use these C tests, clone the [Taiga Project](https://gitlab.com/sfu-rcl/taiga-project) repository. 

Move riscv-opc.c and riscv-opc.h into tool-chain/binutils-gdb/opcodes and tool-chain/binutils-gdb/include/opcode respectively. 

Rebuild the toolchain with the instructions given in the [Adding Functional Unit](https://gitlab.com/sfu-rcl/taiga-project/-/wikis/Adding-Functional-Units-and-Respecitve-Custom-Instructions) section of the repository. 

# Verilator Tests
Replace the Taiga repository in the Taiga Project repository with this one.

Copy the code in any of the C files in `c_tests/verilator` folder into `benchmarks/taiga-example-c-project/src/hello_word.c`. Also copy the contents of the `drivers` folder into `benchmarks/taiga-example-c-project/src/hello_word.c`. 

From the root of the Taiga repository, run: `make run-example-c-project-verilator`, and inspect the logs.

# Zedboard Tests

This section assumes you are able to generate bitstreams for the Taiga system in this repository using the instructions in [Example Hardware Setup](https://gitlab.com/sfu-rcl/taiga-project/-/wikis/Hardware-Setup).

Copy the code in any of the C files in `c_tests/zedboard` folder into `benchmarks/taiga-example-c-project/src/hello_word.c`. Also copy the contents of the `drivers` folder into `benchmarks/taiga-example-c-project/src/hello_word.c`. 

From the root of the Taiga repository, run: `make run-example-c-project-verilator`, and cancel the command after the compilation is done. 

The .hw_init file generated can be used to initialise the Taiga's local memory in the IP Block diagram on Vivado.

<!-- TODO: Add instructions for adding AXI PR request Queue structure -->