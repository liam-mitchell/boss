include include.mk

default: all

.PHONY: iso
all: build initrd iso

$(DEPENDS): $(HEADERS)
	@rm -f .depend
	@for f in $(CSOURCES); \
	do $(CC) $(CFLAGS) -MM $$f | \
	sed "s:\(.*\)\(\.o\):$(OBJDIR)/\1\.c\2:g" >> $(DEPENDS); \
	echo "[DEP] $$f"; \
	done

-include $(DEPENDS)

build: $(DEPENDS) $(OBJFILES)
	@echo "[LD]     bin/kernel.bin"
	@$(CC) $(CFLAGS) $(OBJFILES) $(LDFLAGS) -o bin/kernel.bin

$(OBJDIR)/%.c.o: %.c
	@echo "[CC]     $<"
	@$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(OBJDIR)/%.s.o: %.s
	@echo "[AS]     $<"
	@$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

initrd: $(OBJFILES)
	@echo "[INITRD] $(GEN_INITRD)"
	@cd $(INITRD) && $(GEN_INITRD)

iso: $(OBJFILES)
	@echo "[ISO]    bin/kernel.iso"
	@cp bin/kernel.bin iso/boot/
	@genisoimage -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 -input-charset utf-8 \
	-boot-info-table -o bin/kernel.iso iso

clean:
	@echo "[CLEAN]  $(ISO)"
	@if [ -f $(ISO) ]; then rm $(ISO); fi
	@echo "[CLEAN]  $(BINARY)"
	@if [ -f $(BINARY) ]; then rm $(BINARY); fi
	@echo "[CLEAN]  $(OBJDIR)"
	@rm -f $(OBJFILES) || true
