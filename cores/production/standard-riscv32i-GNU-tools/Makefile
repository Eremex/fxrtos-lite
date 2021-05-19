#
# Makefile for FX-RTOS library.
# Use 'make src' to perform dependency injection and to copy kernel files from
# FX-RTOS sources root location provided by environment variable FXRTOS_DIR.
# Use 'make lib' to create library containing the kernel.
#

GCC_PREFIX ?= riscv64-elf-
C_SRCS = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/*.S)
C_OBJS = $(C_SRCS:.c=.o)
ASM_OBJS = $(ASM_SRCS:.S=.o)

src:
	@[ "${FXDJ}" ] || (echo "FXDJ is not set" ; exit 1)
	@[ "${FXRTOS_DIR}" ] || (echo "FXRTOS_DIR is not set" ; exit 1)
	@echo Performing dependency injection: sources root = $(FXRTOS_DIR)
	mkdir src
	export FX_PREP="$(GCC_PREFIX)gcc -E -Isrc -ffreestanding -include %s %s"; \
	$(realpath $(FXDJ)) -p .,$(FXRTOS_DIR)/components -a default.map -t FXRTOS -o src -l src/fxrtos.lst
	echo '#define FX_INTERFACE(hdr) <hdr.h>' > src/includes.inc
	echo '#define FX_METADATA(data)' >> src/includes.inc

src/%.o : src/%.c
	$(GCC_PREFIX)gcc -pedantic -std=c99 -O2 -Wall -ffunction-sections -march=rv32i -mabi=ilp32 -Isrc -ffreestanding -include includes.inc -c -o $@ $<

src/%.o : src/%.S
	$(GCC_PREFIX)gcc -include includes.inc -march=rv32i -mabi=ilp32  -Isrc -c -o $@ $<

lib : $(C_OBJS) $(ASM_OBJS)
	$(GCC_PREFIX)ar rcs libfxrtos.a $(C_OBJS) $(ASM_OBJS)
	echo '#define FX_INTERFACE(hdr) <stddef.h>' > FXRTOS.h
	echo '#define FX_METADATA(data)' >> FXRTOS.h
	for header in $(addsuffix .h, $(shell cat src/fxrtos.lst)); do cat src/$$header >> FXRTOS.h; done

clean:
	rm -f src/*.o *.tmp FXRTOS.h libfxrtos.a

