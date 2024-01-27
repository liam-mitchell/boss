CC := /home/liam/bin/cross/bin/i686-elf-gcc
AS := nasm

CONFIG ?= opt

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SRCDIR := $(ROOT)/src
SCRIPTDIR := $(ROOT)/scripts
INCDIR := $(SRCDIR)/include

OBJDIR := $(ROOT)/$(CONFIG)/obj
BINDIR := $(ROOT)/$(CONFIG)/bin
ISODIR := $(ROOT)/$(CONFIG)/iso

CSOURCES := $(sort $(shell find $(SRCDIR) -type f -name "*.c"))
ASMSOURCES := $(sort $(shell find $(SRCDIR) -type f -name "*.s"))
HEADERS := $(abspath $(sort $(shell find $(SRCDIR) -type f -name "*.h")))

OBJECTS := $(CSOURCES:.c=.c.o) $(ASMSOURCES:.s=.s.o)

OBJFILES = $(addprefix $(OBJDIR)/, $(notdir $(OBJECTS)))

INCLUDE := -I$(INCDIR)

CFLAGS_COMMON := -std=gnu99 -ffreestanding -Wall -Wextra -Werror $(INCLUDE)
CFLAGS_opt := $(CFLAGS_COMMON) -O2
CFLAGS_dbg := $(CFLAGS_COMMON) -g
LDFLAGS_COMMON := -T $(ROOT)/link.ld -ffreestanding -nostdlib $(INCLUDE)
LDFLAGS_opt := $(LDFLAGS_COMMON) -O2
LDFLAGS_dbg := $(LDFLAGS_COMMON) -g

CFLAGS := $(CFLAGS_$(CONFIG))
LDFLAGS := $(LDFLAGS_$(CONFIG))

ASFLAGS := -felf

VPATH := $(shell find $(SRCDIR) -type d -printf "%p ")

DEPENDS := $(ROOT)/.depend

GEN_INITRD := $(SCRIPTDIR)/gen-initrd.sh
INITRD := $(SCRIPTDIR)/init
INITRD_FILES := $(sort $(shell find $(INITRD) -type f))

INITRD_OUT := $(ISODIR)/boot/initrd.img
BINARY := $(BINDIR)/kernel.bin
ISO := $(BINDIR)/kernel.iso
SYMBOLS := $(BINDIR)/kernel.sym
