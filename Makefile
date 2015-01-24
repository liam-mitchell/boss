include include.mk

default: all

all: $(ISO)

.PHONY: iso initrd build depends
iso: $(ISO)
initrd: $(INITRD_OUT)
build: $(BINARY)
depends: $(DEPENDS)

$(DEPENDS): $(HEADERS)
	@rm -f $(DEPENDS)
	@for f in $(CSOURCES); \
	do $(CC) $(CFLAGS) -MM $$f | \
	sed "s:\(.*\)\(\.o\):$(OBJDIR)/\1\.c\2:g" >> $(DEPENDS); \
	echo "[DEP]    $$f"; \
	done

-include $(DEPENDS)

$(BINARY): $(DEPENDS) $(OBJFILES)
	@echo "[LD]     bin/kernel.bin"
	@$(CC) $(CFLAGS) $(OBJFILES) $(LDFLAGS) -o $(BINARY)

$(OBJDIR)/%.c.o: %.c
	@echo "[CC]     $<"
	@$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(OBJDIR)/%.s.o: %.s
	@echo "[AS]     $<"
	@$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(INITRD_OUT): $(INITRD_FILES)
	@echo "[INITRD] $(GEN_INITRD)"
	@$(GEN_INITRD) -o $(INITRD_OUT) -d $(INITRD) > /dev/null

$(INITRD_FILES):
	;

$(ISO): $(BINARY) $(OBJFILES) $(INITRD_OUT)
	@echo "[ISO]    bin/kernel.iso"
	@cp bin/kernel.bin iso/boot/
	@genisoimage -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 -input-charset utf-8 \
	-boot-info-table -o $(ISO) $(ISODIR)

clean:
	@echo "[CLEAN]  $(ISO)"
	@if [ -f $(ISO) ]; then rm $(ISO); fi
	@echo "[CLEAN]  $(BINARY)"
	@if [ -f $(BINARY) ]; then rm $(BINARY); fi
	@echo "[CLEAN]  $(OBJDIR)"
	@echo "[CLEAN]  $(INITRD_OUT)"
	@rm -rf $(OBJDIR)/* $(INITRD_OUT) || true
