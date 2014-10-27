include include.mk

default: all

.PHONY: iso
all: depend build iso

depend: $(CSOURCES) $(ASMSOURCES)
	$(CC) $(CFLAGS) -MM $^ -MF ./.depend;

.depend:

include .depend

# build: $(OBJECTS)

build: src/*
	@echo "Building..."
	@mkdir -p bin/
	cd src && make
	cd objs && make

iso: src/*
	@echo "Generating iso image..."
	@cp bin/kernel.bin iso/boot/
	@genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
	-boot-info-table -o bin/kernel.iso iso

clean:
	@echo "Removing iso image..."
	@rm bin/kernel.iso || true
	@echo "Removing kernel binaries..."
	@rm bin/kernel.bin iso/boot/kernel.bin || true
	@echo "Removing object files..."
	@cd src && make clean
	@cd objs && make clean
