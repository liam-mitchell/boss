#include "pci.h"

#include "ata.h"
#include "compiler.h"
#include "device_io.h"
#include "macros.h"
#include "memory.h"
#include "printf.h"

#define PCI_PORT_ADDR 0xCF8
#define PCI_PORT_DATA 0xCFC

#define PCI_ADDR_ENABLE (1 << 31)

#define PCI_HTYPE_GEN 0x00
#define PCI_HTYPE_BRIDGE 0x01
#define PCI_HTYPE_CBUS 0x02

#define GET_HTYPE(htype) ((htype) & 0x7F)
#define GET_MULTIFUNC(htype) ((htype) & (1 << 7))

#define PCI_NBUSES 256
#define PCI_NDEVICES 32
#define PCI_NFUNCS 8

static uint32_t pci_conf_address(uint8_t bus, uint8_t device,
                                 uint8_t func, pci_reg_t reg)
{
    return ((uint32_t)bus << 16)
        | ((uint32_t)device << 11)
        | ((uint32_t)func << 8)
        | ((uint32_t)reg)
        | PCI_ADDR_ENABLE;
}

static uint32_t pci_conf_readl(uint8_t bus, uint8_t device,
                               uint8_t func, pci_reg_t reg)
{
    if (reg & 3) {
        PANIC("Tried to read from unaligned address in PCI configuration!");
    }

    uint32_t address = pci_conf_address(bus, device, func, reg);

    outl(PCI_PORT_ADDR, address);
    io_wait();
    uint32_t ret = inl(PCI_PORT_DATA);

    return ret;
}

static uint16_t pci_conf_readw(uint8_t bus, uint8_t device,
                               uint8_t func, pci_reg_t reg)
{
    if (reg & 1) {
        PANIC("Tried to read from unaligned address in PCI configuration!");
    }

    uint8_t word = (reg & 0x2) >> 1;
    reg &= 0xFC;

    uint32_t l = pci_conf_readl(bus, device, func, reg);
    l >>= word * 16;

    return (uint16_t)l;
}

static uint8_t pci_conf_readb(uint8_t bus, uint8_t device,
                              uint8_t func, pci_reg_t reg)
{
    uint8_t byte = reg & 0x3;
    reg &= 0xFC;

    uint32_t l = pci_conf_readl(bus, device, func, reg);
    l >>= byte * 8;

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

static bool detect_multifunc(uint8_t bus, uint8_t device, uint8_t func)
{
    return GET_MULTIFUNC(pci_conf_readb(bus, device, func, PCI_CONF_HTYPE));
}

static uint16_t detect_vendor(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_conf_readw(bus, device, func, PCI_CONF_VENDID);
}

static void read_pci_conf(pci_conf_t *conf, uint8_t bus,
                            uint8_t device, uint8_t func)
{
    int i;
    for (i = 0; i < PCI_HDR_SIZE; i += 4) {
        uint32_t data = pci_conf_readl(bus, device, func, (pci_reg_t)i);
        memcpy(&conf->data[i], &data, sizeof(data));
    }

    while (i < 256) {
        pci_conf_readl(bus, device, func, i);
        i += 4;
    }
}

static void detect_function(uint8_t bus, uint8_t device, uint8_t func)
{
    uint16_t vendor = detect_vendor(bus, device, func);
    uint8_t class = detect_class(bus, device, func);
    uint8_t subclass = detect_subclass(bus, device, func);

    if (vendor != 0xFFFF
        && class == PCI_CLS_MASS_STORAGE
        && subclass == PCI_SUBCLS_IDE)
    {
        printf("Detected PCI IDE mass storage device on bus %x dev %x\n", bus, device);
        read_pci_conf(&ide_bus_conf, bus, device, func);
    }
}

static void detect_functions(uint8_t bus, uint8_t device)
{
    uint8_t func = 0;
    detect_function(bus, device, func);

    if (detect_multifunc(bus, device, func)) {
        for (func = 1; func < PCI_NFUNCS; ++func) {
            detect_function(bus, device, func);
        }
    }
}

static void detect_device(uint8_t bus, uint8_t device)
{
    uint16_t vendor = detect_vendor(bus, device, 0);

    if (vendor != 0xFFFF) {
        detect_functions(bus, device);
    }
}

static void detect_pci_devices()
{
    printf("Detecting PCI devices...\n");
    for (int bus = 0; bus < PCI_NBUSES; ++bus) {
        for (int dev = 0; dev < PCI_NDEVICES; ++dev) {
            detect_device(bus, dev);
        }
    }
}

void init_pci()
{
    detect_pci_devices();
    init_ata();
}

uint8_t get_pci_confb(const pci_conf_t conf, const pci_reg_t reg)
{
    return conf.data[reg];
}

uint16_t get_pci_confw(const pci_conf_t conf, const pci_reg_t reg)
{
    return get_pci_confb(conf, reg) | (get_pci_confb(conf, reg + 1) << 8);
}

uint32_t get_pci_confl(const pci_conf_t conf, const pci_reg_t reg)
{
    return get_pci_confw(conf, reg) | (get_pci_confw(conf, reg + 2) << 16);
}
