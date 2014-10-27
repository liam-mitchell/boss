include ../include.mk

SOURCES=main.o

.PHONY: subdirs $(DIRS)

all: $(DIRS) $(SOURCES)

subdirs: $(DIRS)

$(DIRS):
	@cd $@ && make;

clean:
	@rm *.o || true
	@rm *~ || true

.s.o:
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@ -lgcc
