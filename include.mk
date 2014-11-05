CSOURCES := $(sort $(shell find $(SRCDIR) -type f -name "*.c"))
ASMSOURCES := $(sort $(shell find $(SRCDIR) -type f -name "*.s"))
HEADERS := $(abspath $(sort $(shell find $(SRCDIR) -type f -name "*.h")))

OBJECTS := $(CSOURCES:.c=.c.o) $(ASMSOURCES:.s=.s.o)
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

CC := /home/liam/bin/cross/bin/i686-elf-gcc
AS := nasm

OBJDIR := $(ROOT)/obj
SRCDIR := $(ROOT)/src
BINDIR := $(ROOT)/bin

OBJFILES := $(addprefix $(OBJDIR)/, $(notdir $(OBJECTS)))

INCLUDE := $(addprefix -I, $(shell find $(SRCDIR) -type d -print))

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror $(INCLUDE)
LDFLAGS := -T $(ROOT)/link.ld -ffreestanding -O2 -nostdlib $(INCLUDE)
ASFLAGS := -felf

VPATH := $(shell find $(SRCDIR) -type d -printf "%p ")

DEPENDS := $(ROOT)/.depend
