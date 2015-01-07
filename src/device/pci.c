#include "pci.h"

#include "compiler.h"
#include "device_io.h"
#include "macros.h"
#include "printf.h"

#define PCI_PORT_ADDR 0xCF8
#define PCI_PORT_DATA 0xCFC

#define PCI_ADDR_ENABLE (1 << 31)

#define PCI_CONF_DEVID 0x00
#define PCI_CONF_VENDID 0x02
#define PCI_CONF_STATUS 0x04
#define PCI_CONF_CMD 0x06
#define PCI_CONF_CLS 0x08
#define PCI_CONF_SUBCLS 0x09
#define PCI_CONF_PROGIF 0x0A
#define PCI_CONF_REVID 0x0B
#define PCI_CONF_BIST 0x0C
#define PCI_CONF_HTYPE 0x0D
#define PCI_CONF_LATENCY 0x0E
#define PCI_CONF_CACHELINE 0x0F

#define PCI_HTYPE_GEN 0x00
#define PCI_HTYPE_BRIDGE 0x01
#define PCI_HTYPE_CBUS 0x02

#define GET_HTYPE(htype) ((htype) & 0x7F)
#define GET_MULTIFUNC(htype) ((htype) & (1 << 7))

#define PCI_GEN_BAR0 0x10
#define PCI_GEN_BAR1 0x14
#define PCI_GEN_BAR2 0x18
#define PCI_GEN_BAR3 0x1C
#define PCI_GEN_BAR4 0x20
#define PCI_GEN_BAR5 0x24
#define PCI_GEN_CBUS_CIS 0x28
#define PCI_GEN_SUBID 0x2C
#define PCI_GEN_SUBVENDID 0x2E
#define PCI_GEN_ROM 0x30
#define PCI_GEN_CAP 0x37
#define PCI_GEN_MAX_LATENCY 0x3C
#define PCI_GEN_MIN_GRANT 0x3D
#define PCI_GEN_IPIN 0x3E
#define PCI_GEN_ILINE 0x3F

#define PCI_BUSES 256
#define PCI_DEVICES 32

static uint32_t pci_conf_address(uint8_t bus, uint8_t device,
                                 uint8_t func, uint8_t offset)
{
    return ((uint32_t)bus << 16)
        | ((uint32_t)device << 11)
        | ((uint32_t)func << 8)
        | ((uint32_t)offset)
        | PCI_ADDR_ENABLE;
}

static uint32_t pci_conf_readl(uint8_t bus, uint8_t device,
                               uint8_t func, uint8_t offset)
{
    if (offset & 3) {
        PANIC("Tried to read from unaligned address in PCI configuration!");
    }

    uint32_t address = pci_conf_address(bus, device, func, offset);

    outl(PCI_PORT_ADDR, address);
    io_wait();
    uint32_t ret = inl(PCI_PORT_DATA);
    return ret;
}

static uint16_t pci_conf_readw(uint8_t bus, uint8_t device,
                               uint8_t func, uint8_t offset)
{
    if (offset & 1) {
        PANIC("Tried to read from unaligned address in PCI configuration!");
    }

    uint8_t word = (offset & 0x2) >> 1;
    offset &= 0xFC;

    uint32_t l = pci_conf_readl(bus, device, func, offset);
    l >>= word * 16;

    return (uint16_t)l;
}

static uint8_t pci_conf_readb(uint8_t bus, uint8_t device,
                              uint8_t func, uint8_t offset)
{
    uint8_t byte = offset & 0x3;
    offset &= 0xFC;

    uint32_t l = pci_conf_readl(bus, device, func, offset);
    l >>= byte * 4;

    return (uint8_t)l;
}

static uint8_t detect_class(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_conf_readb(bus, device, func, PCI_CONF_CLS);
}

static uint8_t detect_subclass(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_conf_readb(bus, device, func, PCI_CONF_SUBCLS);
}

static uint8_t detect_devid(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_conf_readw(bus, device, func, PCI_CONF_DEVID);
}

static uint16_t detect_htype(uint8_t bus, uint8_t device, uint8_t func)
{
    return GET_HTYPE(pci_conf_readb(bus, device, func, PCI_CONF_HTYPE));
}

static uint16_t detect_vendor(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_conf_readw(bus, device, func, PCI_CONF_VENDID);
}

static void detect_function(uint8_t bus, uint8_t device, uint8_t func)
{
    uint16_t vendor = detect_vendor(bus, device, func);
    uint32_t dev = detect_devid(bus, device, func);
    uint8_t class = detect_class(bus, device, func);
    uint8_t subclass = detect_subclass(bus, device, func);
    uint8_t type = detect_htype(bus, device, func);
    
    if (vendor != 0xFFFF) {
        printf("Found PCI function:\n"
               " vendor %x\n"
               " device %x\n"
               " class %x\n"
               " subclass %x\n"
               " type %x\n",
               (uint32_t)vendor,
               (uint32_t)dev,
               (uint32_t)class,
               (uint32_t)subclass,
               (uint32_t)type);
    }
}

static void detect_functions(uint8_t bus, uint8_t device)
{
    uint8_t func = 0;
    detect_function(bus, device, func);

    uint8_t type = detect_htype(bus, device, func);
    if (GET_MULTIFUNC(type)) {
        for (func = 1; func < 8; ++func) {
            detect_function(bus, device, func);
        }
    }
}

static void detect_device(uint8_t bus, uint8_t device)
{
    uint16_t vendor = detect_vendor(bus, device, 0);
    /* printf("Device has vendor %x\n", (uint32_t)vendor); */
    if (vendor != 0xFFFF) {
        printf("Found PCI device: vendor %x\n"
               "Detecting functions...\n",
               (uint32_t)vendor);
        detect_functions(bus, device);
    }
}

static void detect_pci_devices()
{
    printf("Detecting PCI devices...\n");
    for (int bus = 0; bus < PCI_BUSES; ++bus) {
        for (int dev = 0; dev < PCI_DEVICES; ++dev) {
            detect_device(bus, dev);
        }
    }
}

void init_pci()
{
    asm volatile ("cli");
    detect_pci_devices();
    asm volatile ("sti");
}
