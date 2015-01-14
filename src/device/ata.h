#ifndef __ATA_H_
#define __ATA_H_

#include <stdint.h>

typedef enum {
    ATA_TYPE_NONE,
    ATA_TYPE_ATA,
    ATA_TYPE_ATAPI,
} ata_type_t;

typedef enum {
    ATA_MASTER = 0xA0,
    ATA_SLAVE = 0xB0
} ata_drvtype_t;

typedef enum {
    ATA_ADDR_CHS,
    ATA_ADDR_LBA28,
    ATA_ADDR_LBA48
} ata_addrmode_t;

typedef struct ata_drive {
    ata_type_t type;
    ata_addrmode_t mode;
    uint64_t size;
    char model[41];
} ata_drive_t;

typedef struct prd {
    uint32_t buf_phys;
    uint16_t size;
    uint16_t last;
} prd_t;

typedef struct ata_bus {
    uint16_t port_data;
    uint16_t port_cmd;
    uint16_t port_bmide;
    ata_drive_t drives[2];
    ata_drvtype_t active_drive;
    uint16_t irq;
    prd_t *prdt;
    uint32_t prdt_phys;
} ata_bus_t;

void init_ata();

void ata_read_sectors(uint32_t buf, const uint64_t lba,
                      const uint32_t n, ata_bus_t *bus,
                      const ata_drvtype_t drive);

void ata_write_sectors(uint32_t buf, const uint64_t lba,
                       const uint32_t n, ata_bus_t *bus,
                       const ata_drvtype_t drive);

#endif /* __ATA_H_ */
