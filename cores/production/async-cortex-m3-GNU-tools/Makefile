#
# Makefile for FX-RTOS library.
# Use 'make src' to perform dependency injection and to copy kernel files from
# FX-RTOS sources root location provided by environment variable FXRTOS_DIR.
# Use 'make' or 'make lib' to create library containing the kernel.
#

GCC_PREFIX ?= arm-none-eabi-
CC=$(GCC_PREFIX)gcc

C_SRCS = $(wildcard src/*.c)
ASM_SRCS = $(wildcard src/*.S)
OBJS = $(C_SRCS:.c=.o) $(ASM_SRCS:.S=.o)

CFLAGS=-pedantic -std=c99 -O2 -Wall -ffunction-sections -mcpu=cortex-m3 -mthumb -Isrc -ffreestanding -include includes.inc
ASFLAGS=-include includes.inc -mcpu=cortex-m3 -Isrc

MAP_FILE ?= async-cortex-m3-gnu.map

all:
	${MAKE} src
	${MAKE} lib

lib: $(OBJS)
	$(GCC_PREFIX)ar rcs libfxrtos.a $(OBJS)
	echo '#define FX_INTERFACE(hdr) <stddef.h>' > FXRTOS.h
	echo '#define FX_METADATA(data)' >> FXRTOS.h
	for header in $(addsuffix .h, $(shell cat src/fxrtos.lst)); do cat src/$$header >> FXRTOS.h; done

src:
	@[ "${FXDJ}" ] || (echo "FXDJ is not set" ; exit 1)
	@[ "${FXRTOS_DIR}" ] || (echo "FXRTOS_DIR is not set" ; exit 1)
	@echo Performing dependency injection: sources root = $(FXRTOS_DIR)
	mkdir src
	export FX_PREP="$(GCC_PREFIX)gcc -E -Isrc -ffreestanding -include %s %s"; \
	$(realpath $(FXDJ)) -p .,$(FXRTOS_DIR)/components -a $(MAP_FILE) -t FXRTOS -o src -l src/fxrtos.lst || (rmdir src; exit 1)
	echo '#define FX_INTERFACE(hdr) <hdr.h>' > src/includes.inc
	echo '#define FX_METADATA(data)' >> src/includes.inc

.PHONY: clean
clean:
	rm -f $(OBJS) *.tmp FXRTOS.h libfxrtos.a

