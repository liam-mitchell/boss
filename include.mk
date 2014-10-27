CSOURCES := $(shell find $(SRCDIR) -type f -name "*.c")
ASMSOURCES := $(shell find $(SRCDIR) -type f -name "*.S")
OBJECTS := $(CSOURCES:.c=.o) $(ASMSOURCES:.S=.o)

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

CC := /home/liam/bin/cross/bin/i686-elf-gcc
AS := nasm

OBJDIR := $(ROOT)/objs
SRCDIR := $(ROOT)/src
BINDIR := $(ROOT)/bin
IDIR := $(SRC)/temp
DIRS := $(subst .git,,$(subst examples/,,$(wildcard */)))

INCLUDE := $(addprefix -I, $(shell find $(SRCDIR) -type d -print))

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra $(INCLUDE)
LDFLAGS := -T $(ROOT)/link.ld -ffreestanding -O2 -nostdlib $(INCLUDE)
ASFLAGS := -felf
