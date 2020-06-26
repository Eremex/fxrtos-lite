README.MD

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

ARM version supports GCC, Keil MDK and IAR EWARM compilers.
RISC-V version supports only GCC at now.

Host system may be either Windows or Linux (Mac should also work, but untested).
No external dependencies required except the compiler.

Getting started
---------------

For those who do not want to mess with toolchains and source code we provide prebuilt binaries (see demo folder in 'preconfigured' kernel repository). ARM version is compatible with any Cortex-M3+ chip and may
be used as just two files: header and library.

While binary version may be sufficient for most users it lacks configuration and optimization options. The OS may be either rebuilt as a binary or it can be added as file set in IDE like Keil/IAR.

Compiler installation:

Windows:

https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

Linux:

	sudo apt install binutils-arm-none-eabi gcc-arm-none-eabi

How to build the library from sources:
- Ensure supported compiler is available via PATH
- Set GCC_PREFIX variable for GCC (arm-none-eabi- for ARM)
- Enter directory for target core (i.e. standard-cortex-m3)
- Run build.bat on Windows (or make lib on Linux/Mac (ARM only))

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
