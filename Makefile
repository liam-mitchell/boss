include include.mk

default: all

.PHONY: iso
all: build iso

$(DEPENDS): $(HEADERS)
	@rm -f .depend
	@for f in $(CSOURCES); \
	do $(CC) $(CFLAGS) -MM $$f | \
	sed "s:\(.*\)\(\.o\):$(OBJDIR)/\1\.c\2:g" >> $(DEPENDS); \
	echo "[DEP] $$f"; \
	done

-include $(DEPENDS)

build: $(DEPENDS) $(OBJFILES)
	@echo -n "[LD]  "
	$(CC) $(CFLAGS) $(OBJFILES) $(LDFLAGS) -o bin/kernel.bin

$(OBJDIR)/%.c.o: %.c
	@echo -n "[CC]  "
	$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

$(OBJDIR)/%.s.o: %.s
	@echo -n "[AS]  "
	$(AS) $(ASFLAGS) $< -o $(OBJDIR)/$(shell basename $@)

iso: $(OBJFILES)
	@echo -n "[ISO] "
	@cp bin/kernel.bin iso/boot/
	genisoimage -R -b boot/grub/stage2_eltorito \
	-no-emul-boot -boot-load-size 4 \
	-boot-info-table -o bin/kernel.iso iso

clean:
	@echo -n "[RM]  "
	if [ -f $(BINDIR)/kernel.iso ]; then rm $(BINDIR)/kernel.iso; fi
	@echo -n "[RM]  "
	if [ -f $(BINDIR)/kernel.bin ]; then rm $(BINDIR)/kernel.bin; fi
	@echo -n "[RM]  "
	rm -f $(OBJFILES) || true
