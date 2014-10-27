include ../../include.mk

SOURCES=terminal.o device_io.o timer.o

all: $(SOURCES)

clean:
	@echo "Removing objects..."
	@rm *.o || true
	@rm *~ || true

.s.o:
	@echo "Assembling $<"
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$@

.S.o:
	@echo "Assembling $<"
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$@

.c.o:
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@ -lgcc
