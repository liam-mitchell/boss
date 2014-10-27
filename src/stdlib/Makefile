include ../../include.mk

SOURCES=memory.o string.o

all: $(SOURCES)
	echo "include: " $(INCLUDE)
	echo "srcdir: " $(SRCDIR)
	echo "shell find etc: " $(shell find $(SRCDIR) -type d -print)

clean:
	@echo "Removing objects..."
	@rm *.o || true
	@rm *~ || true

.s.o:
	@echo "Assembling $<"
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$@

.c.o:
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@ -lgcc
