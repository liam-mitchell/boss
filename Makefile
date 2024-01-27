include include.mk

default: all

all: dirs $(SYMBOLS) $(ISO)

.PHONY: iso initrd build depends dirs
iso: $(ISO)
initrd: $(INITRD_OUT)
build: $(BINARY)
depends: $(DEPENDS)

dirs: $(OBJDIR) $(BINDIR) $(ISODIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)
$(BINDIR):
	mkdir -p $(BINDIR)
$(ISODIR):
	mkdir -p $(ISODIR)/boot

$(DEPENDS): $(HEADERS)
	@rm -f $(DEPENDS)
	@for f in $(CSOURCES); \
	do $(CC) $(CFLAGS) -MM $$f | \
	sed "s:\(.*\)\(\.o\):$(OBJDIR)/\1\.c\2:g" >> $(DEPENDS); \
	echo "[DEP]    $$f"; \
	done

-include $(DEPENDS)

$(BINARY): $(DEPENDS) $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) $(LDFLAGS) -o $(BINARY)

$(SYMBOLS): $(BINARY)
ifeq ($(CONFIG),dbg)
	objcopy --only-keep-debug $(BINARY) $(SYMBOLS)
endif

$(OBJDIR)/%.c.o: %.c
	$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(OBJDIR)/%.s.o: %.s
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(INITRD_OUT): $(INITRD_FILES)
	$(GEN_INITRD) -o $(INITRD_OUT) -d $(INITRD) > /dev/null

$(INITRD_FILES):
	;

$(ISO): $(BINARY) $(OBJFILES) $(INITRD_OUT)
	cp $(BINARY) $(ISODIR)/boot/
	cp -R grub/ $(ISODIR)/boot
	genisoimage -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 -input-charset utf-8 \
	-boot-info-table -o $(ISO) $(ISODIR)

clean:
	rm -rf $(ROOT)/$(CONFIG)

cleanall:
	rm -rf $(ROOT)/opt
	rm -rf $(ROOT)/dbg
