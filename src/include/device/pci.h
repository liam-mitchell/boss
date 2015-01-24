#ifndef __PCI_H_
#define __PCI_H_

#include <stdint.h>

typedef enum {
    PCI_CONF_VENDID = 0x00,
    PCI_CONF_DEVID = 0x02,
    PCI_CONF_CMD = 0x04,
    PCI_CONF_STATUS = 0x06,
    PCI_CONF_REVID = 0x08,
    PCI_CONF_PROGIF = 0x09,
    PCI_CONF_SUBCLS = 0x0A,
    PCI_CONF_CLS = 0x0B,
    PCI_CONF_CACHELINE = 0x0C,
    PCI_CONF_LAT_TMR = 0x0D,
    PCI_CONF_HTYPE = 0x0E,
    PCI_CONF_BIST = 0x0F,
    PCI_GEN_BAR0 = 0x10,
    PCI_GEN_BAR1 = 0x14,
    PCI_GEN_BAR2 = 0x18,
    PCI_GEN_BAR3 = 0x1C,
    PCI_GEN_BAR4 = 0x20,
    PCI_GEN_BAR5 = 0x24,
    PCI_GEN_CBUS_CIS = 0x28,
    PCI_GEN_SUBVENDID = 0x2C,
    PCI_GEN_SUBID = 0x2E,
    PCI_GEN_ROM = 0x30,
    PCI_GEN_CAP = 0x34,
    PCI_GEN_ILINE = 0x3C,
    PCI_GEN_IPIN = 0x3D,
    PCI_GEN_MIN_GNT = 0x3E,
    PCI_GEN_MAX_LAT = 0x3F,
} pci_reg_t;

#define PCI_HDR_SIZE (PCI_GEN_MAX_LAT + 1)

#define PCI_IDE_NBUSES 2

#define PCI_CLS_MASS_STORAGE 0x01
#define PCI_SUBCLS_IDE 0x01

typedef struct pci_conf {
    uint8_t data[PCI_HDR_SIZE];
} pci_conf_t;

pci_conf_t ide_bus_conf;

void init_pci();
uint8_t get_pci_confb(const pci_conf_t conf, const pci_reg_t reg);
uint16_t get_pci_confw(const pci_conf_t conf, const pci_reg_t reg);
uint32_t get_pci_confl(const pci_conf_t conf, const pci_reg_t reg);

#endif // __PCI_H_
