<a href="https://scan.coverity.com/projects/eremex-fxrtos-lite">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/23951/badge.svg"/>
</a>
<a href="https://github.com/eremex/fxrtos-lite/actions">
  <img alt="Build Status"
       src="https://github.com/eremex/fxrtos-lite/actions/workflows/build.yml/badge.svg"/>
</a>

Description
-----------

FX-RTOS Lite is small kernel intended to be used with microcontrollers.
It is implemented as a C99 static library containing OS services.

Features
--------

API includes the following services:
- threads
- software timers (one-shot and periodic)
- semaphores
- mutexes with priority ceiling
- message queues
- memory block pools
- events
- condition variables
- barriers
- arbitrary-sized memory pools
- rwlocks

Supported hardware and toolchains
---------------------------------

Lite version supports only two most common CPU architectures: 
- ARM Cortex-M3+ (ARMv7-M architecture, may also be used on ARMv8-M)
- RISC-V (RV32I profile)

### Current configurations:
 Release | Platform
:--- | :---
`async-cortex-m3-GNU-tools.zip` | Minimal OS using event service routines instead threads for cortex-m3+ MCU's
`standard-cortex-m3.zip` | Regular configuration for cortex-m3+ MCU's
`standard-riscv32i-GNU-tools.zip` | Regular configuration for RISC-V rv32i ISA

ARM version supports GCC, Keil MDK and IAR EWARM compilers.
RISC-V version supports only GCC at now.

Host system may be either Windows or Linux (Mac should also work, but untested).
No external dependencies required except the compiler.

Getting started
---------------

This repository contains non-configured version of the kernel represented as a set of components. It is useful if you want to contribute to OS. 
In case if you just need a kernel to use in your embedded application consider using preconfigured kernels available for ARM and RISC-V.

### How to build the library from preconfigured sources:

- Download and unpack appropriate release archive from [Releases](https://github.com/Eremex/fxrtos-lite/releases)
- Ensure supported compiler is available via PATH
- Set GCC_PREFIX as compiler prefix if you use GCC (i.e. 'riscv-none-embed-')
- Enter directory where build.bat is located
- Run 'build.bat'

For those who do not want to mess with toolchains and source code we provide prebuilt binaries. While binary version may be sufficient for most users it lacks configuration and optimization options.

### How to build the library from scratch:

- Ensure [fx-dj.py](https://github.com/Eremex/fx-dj) script is available via PATH
- Ensure supported compiler is available via PATH
- Set environment variables
    - FXRTOS_DIR as path to kernel root folder
    - GCC_PREFIX as compiler prefix if you use GCC (i.e. 'arm-none-eabi-' for ARM)
    - FXDJ as dependency injection tool (i.e. 'fx-dj.py')
- Enter directory for target core (e.g. 'cores\standard-cortex-m3')
- Run 'build.bat' on Windows or 'make src' and then 'make lib' on Linux/Mac (ARM only)

### How to use
The OS may be linked to the project as a binary or it can be added as set of source files in IDE.
Demonstrations of use are located in [fxrtos-examples](https://github.com/Eremex/fxrtos-examples) repo.

Limitations
-----------

Please note that Lite version is a soft-realtime (or best-effort) kernel. It is NOT intended for deterministic hard-realtime operation.
Advanced features such as deterministic timers, low-latency deferred interrupt processing, multiprocessing support, privilege levels separation and security
are available only in full version. Please, contact EREMEX sales department for further details.

When developing latency-critical applications using Lite edition the following limitations should been taken into account:

- Broadcasting (or "notify all") synchronization objects such as condvar or event. More waiting threads results in longer latency since notification process is non-preemptive. Possible solutions: do not use broadcasting objects or limit maximum number of waiting threads.
- Priority-based notification policy with synchronization objects (i.e. posting semaphore and releasing the most prioritized waiting thread) uses linear search with scheduling disabled. This means that N waiting threads results in unbounded O(n) scheduling and interrupt latency. Possible solutions: use FIFO notification policy or limit maximum number of waiting threads for any synchronization object.
- Message queue flush releases up to N threads where N is queue length. Possible solutions: do not use queue flushing or limit queue length to reasonable value.
- Timers implementation uses sorted queues and linear search resulting in O(n) latency depending on number of active timers in the system. Possible solutions: Limit maximum number of software timers and do not use timeslicing.

Support
-----------
For questions on using FX-RTOS, contact authors via telegram group (https://t.me/fxrtos).
