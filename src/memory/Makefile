include ../../include.mk

SOURCES=mem-detect.o mm.o

all: $(SOURCES)

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
