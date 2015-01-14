#include "ata.h"

#include "bool.h"
#include "bits.h"
#include "compiler.h"
#include "interrupt.h"
#include "kheap.h"
#include "macros.h"
#include "memory.h"
#include "pci.h"
#include "port.h"
#include "printf.h"
#include "string.h"
#include "vmm.h"

#define ATA_CHAN_PRIM 0
#define ATA_CHAN_SEC 1

#define ATA_DRV_MASTER 0
#define ATA_DRV_SLAVE 1

#define ATA_PORT_PRI_DATA 0x1F0
#define ATA_PORT_PRI_CTL 0x3F6
#define ATA_PORT_SEC_DATA 0x170
#define ATA_PORT_SEC_CTL 0x376

#define ATA_PORT_DATA 0x00
#define ATA_PORT_ERROR 0x01
#define ATA_PORT_FEATURES 0x01
#define ATA_PORT_SECCOUNT 0x02
#define ATA_PORT_LBALOW 0x03
#define ATA_PORT_LBAMID 0x04
#define ATA_PORT_LBAHIGH 0x05
#define ATA_PORT_DEVICE 0x06
#define ATA_PORT_STATUS 0x07
#define ATA_PORT_CMD 0x07

#define ATA_PORT_CONTROL 0x00
#define ATA_PORT_ALTSTATUS 0x00

#define ATAPI_PORT_DATA 0x00
#define ATAPI_PORT_ERROR 0x01
#define ATAPI_PORT_FEATURES 0x01
#define ATAPI_PORT_SECCOUNT 0x02
#define ATAPI_PORT_INTREASON 0x02
#define ATAPI_PORT_LBALOW 0x03
#define ATAPI_PORT_BYTESLOW 0x04
#define ATAPI_PORT_BYTESHIGH 0x05
#define ATAPI_PORT_DEVSEL 0x06
#define ATAPI_PORT_STATUS 0x07
#define ATAPI_PORT_CMD 0x07

#define ATAPI_PORT_CONTROL 0x00
#define ATAPI_PORT_ALTSTATUS 0x00

#define ATAPI_SIG_MID 0x14
#define ATAPI_SIG_HIGH 0xEB

#define ATA_STAT_ERR (1 << 0)
#define ATA_STAT_DRQ (1 << 3)
#define ATA_STAT_SRV (1 << 4)
#define ATA_STAT_DF (1 << 5)
#define ATA_STAT_RDY (1 << 6)
#define ATA_STAT_BSY (1 << 7)

#define ATA_CTL_NIEN (1 << 1)
#define ATA_CTL_SRST (1 << 2)
#define ATA_CTL_HOB (1 << 7)

#define ATA_ID_DEVTYPE 0
#define ATA_ID_CYLINDERS 1
#define ATA_ID_HEADS 3
#define ATA_ID_SECTORS 6
#define ATA_ID_SERIAL 10
#define ATA_ID_MODEL 27
#define ATA_ID_CAP 49
#define ATA_ID_FIELDVALID 53
#define ATA_ID_MAX_LBA 60
#define ATA_ID_CSETS 83
#define ATA_ID_MAX_LBA_EXT 100
#define ATA_ID_SIG 255

#define ATA_IDBUF_SIZE 256

#define ATA_SIG 0xA5

#define ATA_CSET_LBA48 (1 << 10)

#define ATA_BUS_PRI 0
#define ATA_BUS_SEC 1

#define ATA_WAIT_READS 4

#define ATA_MAX_SIZE_CHS 0xFFFFFFF

#define ATA_BMR_CMD 0x0
#define ATA_BMR_STAT 0x0
#define ATA_BMR_PADDR 0x4

#define ATA_PRDT_SIZE 256

#define ATA_PRDT_LAST (1 << 15)

#define ATA_BMRCMD_DMA (1 << 0)
#define ATA_BMRCMD_READ (1 << 3)

#define ATA_BMRSTAT_DSKIRQ (1 << 4)
#define ATA_BMRSTAT_INDMA (1 << 0)

#define ATA_DEVSEL_LBA (1 << 6)

typedef enum {
    ATA_CMD_READ_PIO = 0x20,
    ATA_CMD_READ_PIO_EXT = 0x24,
    ATA_CMD_READ_DMA = 0xC8,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_PIO = 0x30,
    ATA_CMD_WRITE_PIO_EXT = 0x34,
    ATA_CMD_WRITE_DMA = 0xCA,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
    ATA_CMD_CACHE_FLUSH = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET = 0xA0,
    ATA_CMD_IDENTIFY = 0xEC,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
} ata_cmd_t;

typedef enum {
    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B,
} atapi_cmd_t;

static ata_bus_t buses[PCI_IDE_NBUSES];

static void wait_valid_bus_status(ata_bus_t *bus)
{
    for (int i = 0; i < ATA_WAIT_READS; ++i) {
        inb(bus->port_cmd + ATA_PORT_ALTSTATUS);
    }
}

static void set_drive(ata_bus_t *bus, ata_drvtype_t drv)
{
    if (drv != bus->active_drive) {
        outb(bus->port_data + ATA_PORT_DEVICE, drv);
        bus->active_drive = drv;
        wait_valid_bus_status(bus);
    }
}

static void send_command(ata_bus_t *bus, ata_cmd_t cmd,
                         uint8_t seccount, uint8_t lbalo,
                         uint8_t lbamid, uint8_t lbahi)
{
    /* uint8_t status = 0; */
    /* while (!TST_BITS(status, ATA_STAT_RDY)) { */
    /*     status = inb(bus->port_data + ATA_PORT_STATUS); */
    /* } */

    outb(bus->port_data + ATA_PORT_SECCOUNT, seccount);
    outb(bus->port_data + ATA_PORT_LBALOW, lbalo);
    outb(bus->port_data + ATA_PORT_LBAMID, lbamid);
    outb(bus->port_data + ATA_PORT_LBAHIGH, lbahi);
    outb(bus->port_data + ATA_PORT_CMD, cmd);
}

static void set_drive_type(ata_bus_t *bus, ata_drvtype_t drv, ata_type_t type)
{
    uint8_t d = (drv == ATA_MASTER) ? ATA_DRV_MASTER : ATA_DRV_SLAVE;
    bus->drives[d].type = type;
}

static void set_drive_size(ata_bus_t *bus, ata_drvtype_t drv, uint16_t *ident)
{
    uint8_t d = (drv == ATA_MASTER) ? ATA_DRV_MASTER : ATA_DRV_SLAVE;
    uint16_t csets = ident[ATA_ID_CSETS];

    printf("Drive supports command sets %x\n", csets);

    if (csets & ATA_CSET_LBA48) {
        printf("Drive uses LBA48 addressing\n");
        uint64_t lba;
        memcpy(&lba, ident + ATA_ID_MAX_LBA_EXT, sizeof(lba));
        bus->drives[d].mode = ATA_ADDR_LBA48;
        bus->drives[d].size = lba;
    }
    else {
        uint32_t lba;
        memcpy(&lba, ident + ATA_ID_MAX_LBA, sizeof(lba));
        if (lba != 0) {
            printf("Drive uses LBA28 addressing\n");
            bus->drives[d].mode = ATA_ADDR_LBA28;
            bus->drives[d].size = lba;
        }
        else {
            printf("Drive uses CHS addressing\n");
            bus->drives[d].mode = ATA_ADDR_CHS;
            bus->drives[d].size = ATA_MAX_SIZE_CHS;
        }
    }
}

static void set_drive_model(ata_bus_t *bus, ata_drvtype_t drv, uint16_t *ident)
{
    uint8_t d = (drv == ATA_MASTER) ? ATA_DRV_MASTER : ATA_DRV_SLAVE;
    /* memcpy(bus->drives[d].model, ident + ATA_ID_MODEL, 40); */

    for (uint32_t i = 0; i < 20; ++i) {
        bus->drives[d].model[i * 2] = ident[i] >> 8;
        bus->drives[d].model[i * 2 + 1] = ident[i] & 0xFF;
    }

    bus->drives[d].model[40] = '\0';
}

static bool check_identify_integrity(uint16_t *buf)
{
    int8_t sum = 0;
    for (uint32_t i = 0; i < ATA_IDBUF_SIZE; ++i) {
        sum += buf[i] & 0xFF;
        sum += buf[i] >> 8;
    }

    uint8_t sig = buf[ATA_IDBUF_SIZE - 1] & 0xFF;

    bool good = true;
    if (sum != 0) {
        printf("Checksum error in ATA identify!\n"
               " Should be 0, was %x\n", sum);
        good = false;
    }
    if (sig != ATA_SIG) {
        printf("Signature error in ATA identify!\n"
               " Should be %x, was %x\n", ATA_SIG, sig);
        good = false;
    }

    return good;
}

static void read_identify(ata_bus_t *bus, ata_drvtype_t drv)
{
    uint16_t buf[ATA_IDBUF_SIZE];
    for (uint32_t i = 0; i < ATA_IDBUF_SIZE; ++i) {
        buf[i] = inw(bus->port_data + ATA_PORT_DATA);
    }

    if (!check_identify_integrity(buf)) {
        PANIC("Invalid identify information from ATA drive\n");
    }

    printf("Device model %s\n", (char *)(buf + ATA_ID_MODEL));
    printf("Device serial %s\n", (char *)(buf + ATA_ID_SERIAL));

    set_drive_size(bus, drv, buf);
    set_drive_model(bus, drv, buf);
}

static void identify_packet(ata_bus_t *bus, ata_drvtype_t drv)
{
    set_drive(bus, drv);
    send_command(bus, ATA_CMD_IDENTIFY_PACKET, 0, 0, 0, 0);
    uint8_t status = inb(bus->port_cmd + ATA_PORT_ALTSTATUS);

    if (!status) {
        for (uint32_t i = 0; i < ATA_IDBUF_SIZE; ++i) {
            inw(bus->port_data + ATA_PORT_DATA);
        }
        set_drive_type(bus, drv, ATA_TYPE_NONE);
        return;
    }

    while (TST_BITS(status, ATA_STAT_BSY) && !TST_BITS(status, ATA_STAT_ERR)) {
        status = inb(bus->port_cmd + ATA_PORT_ALTSTATUS);
    }

    if (TST_BITS(status, ATA_STAT_ERR)) {
        for (uint32_t i = 0; i < ATA_IDBUF_SIZE; ++i) {
            inw(bus->port_data + ATA_PORT_DATA);
        }
x        set_drive_type(bus, drv, ATA_TYPE_NONE);
        return;
    }

    set_drive_type(bus, drv, ATA_TYPE_ATAPI);
    read_identify(bus, drv);
}

static void identify(ata_bus_t *bus, ata_drvtype_t drv)
{
    set_drive(bus, drv);
    send_command(bus, ATA_CMD_IDENTIFY, 0, 0, 0, 0);
    uint8_t status = inb(bus->port_cmd + ATA_PORT_ALTSTATUS);

    if (!status) {
        set_drive_type(bus, drv, ATA_TYPE_NONE);
        return;
    }

    while (TST_BITS(status, ATA_STAT_BSY) && !TST_BITS(status, ATA_STAT_ERR)) {
        status = inb(bus->port_cmd + ATA_PORT_ALTSTATUS);
    }

    uint8_t lbamid = inb(bus->port_data + ATA_PORT_LBAMID);
    uint8_t lbahi = inb(bus->port_data + ATA_PORT_LBAHIGH);

    if (lbamid == ATAPI_SIG_MID && lbahi == ATAPI_SIG_HIGH) {
        for (uint32_t i = 0; i < ATA_IDBUF_SIZE; ++i) {
            inw(bus->port_data + ATA_PORT_DATA);
        }

        identify_packet(bus, drv);
        return;
    }

    while (!TST_BITS(status, ATA_STAT_DRQ) && !TST_BITS(status, ATA_STAT_ERR)) {
        status = inb(bus->port_cmd + ATA_PORT_ALTSTATUS);
    }

    if (!TST_BITS(status, ATA_STAT_ERR)) {
        set_drive_type(bus, drv, ATA_TYPE_ATA);
        read_identify(bus, drv);
    }
    else {
        printf("Error encountered when identifying bus\n");
    }
}

/* static void disable_bus_irq(ata_bus_t *bus) */
/* { */
/*     outb(bus->port_cmd + ATA_PORT_CONTROL, ATA_CTL_NIEN); */
/* } */

static void enable_bus_irq(ata_bus_t *bus)
{
    outb(bus->port_cmd + ATA_PORT_CONTROL, 0);
}

static void print_drives(ata_bus_t *bus, const char *msg)
{
    printf("Drives detected on bus %s:\n", msg);

    for (int i = 0; i < 2; ++i) {
        if (bus->drives[i].type == ATA_TYPE_NONE) {
            printf(" No drive %d\n", i);
        }
        else {
            const char *type = (bus->drives[i].type == ATA_TYPE_ATA) ?
                "ATA" : "ATAPI";
        
            printf(" Drive %d: %s (type %s size %x)\n",
                   i,
                   bus->drives[i].model,
                   type,
                   bus->drives[i].size);
        }
    }
}

static void init_busmaster(ata_bus_t *bus)
{
    bus->prdt = kzalloc(MEM_DMA, ATA_PRDT_SIZE);
    bus->prdt_phys = get_physical((uint32_t)bus->prdt);

    printf("Initialized busmaster with PRDT virt %x phys %x\n", bus->prdt, bus->prdt_phys);

    outl(bus->port_bmide + ATA_BMR_PADDR, bus->prdt_phys);
}

static void init_drives()
{
    pci_conf_t conf = ide_bus_conf;

    buses[ATA_BUS_PRI].port_data = get_pci_confl(conf, PCI_GEN_BAR0);
    buses[ATA_BUS_PRI].port_cmd = get_pci_confl(conf, PCI_GEN_BAR1);
    buses[ATA_BUS_PRI].port_bmide = get_pci_confl(conf, PCI_GEN_BAR4);
    buses[ATA_BUS_PRI].irq = 14; /* TODO: detect this properly in PCI init */

    if (buses[ATA_BUS_PRI].port_data == 0) {
        buses[ATA_BUS_PRI].port_data = ATA_PORT_PRI_DATA;
    }

    if (buses[ATA_BUS_PRI].port_cmd == 0) {
        buses[ATA_BUS_PRI].port_cmd = ATA_PORT_PRI_CTL;
    }

    buses[ATA_BUS_SEC].port_data = get_pci_confl(conf, PCI_GEN_BAR2);
    buses[ATA_BUS_SEC].port_cmd = get_pci_confl(conf, PCI_GEN_BAR3);
    buses[ATA_BUS_SEC].port_bmide = get_pci_confl(conf, PCI_GEN_BAR4) + 8;
    buses[ATA_BUS_SEC].irq = 15; /* TODO: see above */

    if (buses[ATA_BUS_SEC].port_data == 0) {
        buses[ATA_BUS_SEC].port_data = ATA_PORT_SEC_DATA;
    }

    if (buses[ATA_BUS_SEC].port_cmd == 0) {
        buses[ATA_BUS_SEC].port_cmd = ATA_PORT_SEC_CTL;
    }

    /* disable_bus_irq(&buses[ATA_BUS_PRI]); */
    /* disable_bus_irq(&buses[ATA_BUS_SEC]); */

    for (int i = 0; i < PCI_IDE_NBUSES; ++i) {
        identify(&buses[i], ATA_MASTER);
        identify(&buses[i], ATA_SLAVE);
        /* printf("Identified drives on bus %d\n", i); */
        init_busmaster(&buses[i]);
    }

    print_drives(&buses[ATA_BUS_PRI], "primary");
    print_drives(&buses[ATA_BUS_SEC], "secondary");

    enable_bus_irq(&buses[ATA_BUS_PRI]);
    enable_bus_irq(&buses[ATA_BUS_SEC]);
}

/* static void read_atapi_sectors(void *buf, const uint64_t lba, */
/*                                const uint32_t n, const ata_bus_t *bus, */
/*                                const ata_drvtype_t drive) */
/* { */

/* } */

static void set_dma_command_bits(ata_bus_t *bus, uint8_t bits)
{
    uint8_t cmd = inb(bus->port_bmide + ATA_BMR_CMD);
    SET_BITS(cmd, bits);
    outb(bus->port_bmide + ATA_BMR_CMD, cmd);
}

static void clear_dma_command_bits(ata_bus_t *bus, uint8_t bits)
{
    uint8_t cmd = inb(bus->port_bmide + ATA_BMR_CMD);
    CLR_BITS(cmd, bits);
    outb(bus->port_bmide + ATA_BMR_CMD, cmd);
}

static void stop_dma(ata_bus_t *bus)
{
    clear_dma_command_bits(bus, ATA_BMRCMD_DMA);
}

static void start_dma(ata_bus_t *bus)
{
    set_dma_command_bits(bus, ATA_BMRCMD_DMA);
}

static void set_bus_dma_read(ata_bus_t *bus)
{
    set_dma_command_bits(bus, ATA_BMRCMD_READ);
}

static void set_bus_dma_write(ata_bus_t *bus)
{
    clear_dma_command_bits(bus, ATA_BMRCMD_READ);
}

static void set_prdt(ata_bus_t *bus, uint32_t buf, uint32_t nsectors)
{
    uint32_t prd = 0;
    while (nsectors > 256) {
        bus->prdt[prd].buf_phys = buf;
        bus->prdt[prd].size = 0xFFFF;
        bus->prdt[prd].last = 0;

        nsectors -= 256;
        ++prd;
        buf += 0xFFFF;
    }

    bus->prdt[prd].buf_phys = buf;
    bus->prdt[prd].size = nsectors * 256;
    bus->prdt[prd].last = ATA_PRDT_LAST;
}

static void transfer_prd(const uint64_t lba, ata_bus_t *bus,
                         ata_cmd_t cmd, prd_t *prd)
{
    uint8_t seccount = prd->size / 256;
    uint8_t lbalow = lba & 0xFF;
    uint8_t lbamid = (lba >> 8) & 0xFF;
    uint8_t lbahi = (lba >> 16 & 0xFF);
    uint8_t lbatop = (lba >> 24) & 0xF;

    uint8_t devsel = inb(bus->port_data + ATA_PORT_DEVICE);
    devsel |= lbatop;
    devsel |= ATA_DEVSEL_LBA;
    outb(bus->port_data + ATA_PORT_DEVICE, devsel);

    printf("Sending transfer command %x:\n"
           " low %x\n"
           " mid %x\n"
           " hi %x\n"
           " devsel %x\n"
           " seccount %x\n",
           cmd, lbalow, lbamid, lbahi, devsel, seccount);
    printf("PRD entry:\n"
           " phys: %x\n"
           " size: %x\n"
           " last: %x\n", prd->buf_phys, prd->size, prd->last);
        

    send_command(bus, cmd, seccount, lbalow, lbamid, lbahi);
}

static void transfer_dma_sectors(const uint64_t lba, ata_bus_t *bus,
                                 ata_cmd_t cmd)
{
    if (lba > ATA_MAX_SIZE_CHS) {
        printf("Only LBA28 addressing is supported!\n");
        return;
    }

    outb(bus->port_data + ATA_PORT_FEATURES, 1);

    prd_t *prd = bus->prdt;
    bool done = false;
    while (!done) {
        done = prd->last;
        transfer_prd(lba, bus, cmd, prd);
        ++prd;
    }
}

static void read_dma_sectors(const uint64_t lba, ata_bus_t *bus)
{
    transfer_dma_sectors(lba, bus, ATA_CMD_READ_DMA);
}

static void write_dma_sectors(const uint64_t lba, ata_bus_t *bus)
{
    transfer_dma_sectors(lba, bus, ATA_CMD_WRITE_DMA);
}

static bool irq_primary = false;
static bool irq_secondary = false;

static void ata_irq(ata_bus_t *bus, const char *msg)
{
    printf("ATA IRQ recieved: %s\n", msg);

    uint8_t status = inb(bus->port_bmide + ATA_BMR_STAT);
    if (!TST_BITS(status, ATA_BMRSTAT_DSKIRQ)) {
        printf(" IRQ was from device other than ATA drive!\n");
    }
    else if (!TST_BITS(status, ATA_BMRSTAT_INDMA)) {
        printf(" Completed DMA transfer!\n");
    }
}

static void ata_primary_irq(registers_t  __unused *regs)
{
    ata_irq(&buses[ATA_BUS_PRI], "primary");
    irq_primary = 1;
}

static void ata_secondary_irq(registers_t __unused *regs)
{
    ata_irq(&buses[ATA_BUS_SEC], "secondary");
    irq_secondary = 1;
}

static void wait_ata_primary()
{
    while (!irq_primary) { /* wait */ }
    irq_primary = 0;
}

static void wait_ata_secondary()
{
    while (!irq_secondary) { /* wait */ printf("waiting for secondary irq...\n");}
    irq_secondary = 0;
}

static void read_ata_sectors(uint32_t buf, const uint64_t lba,
                             const uint32_t n, ata_bus_t *bus,
                             const ata_drvtype_t drive)
{
    set_drive(bus, drive);
    set_prdt(bus, buf, n);

    stop_dma(bus);
    set_bus_dma_read(bus);
    start_dma(bus);

    read_dma_sectors(lba, bus);

    if (bus == &buses[ATA_BUS_PRI]) {
        wait_ata_primary();
    }
    else {
        wait_ata_secondary();
    }
}

static void write_ata_sectors(uint32_t buf, const uint64_t lba,
                              const uint32_t n, ata_bus_t *bus,
                              const ata_drvtype_t drive)
{
    set_drive(bus, drive);
    set_prdt(bus, buf, n);

    stop_dma(bus);
    set_bus_dma_write(bus);
    start_dma(bus);

    write_dma_sectors(lba, bus);

    if (bus == &buses[ATA_BUS_PRI]) {
        wait_ata_primary();
    }
    else {
        wait_ata_secondary();
    }
}

void ata_read_sectors(uint32_t buf, const uint64_t lba,
                      const uint32_t n, ata_bus_t *bus,
                      const ata_drvtype_t drive)
{
    uint8_t d = (drive == ATA_MASTER) ? ATA_DRV_MASTER : ATA_DRV_SLAVE;

    if (bus->drives[d].type == ATA_TYPE_ATA) {
        read_ata_sectors(buf, lba, n, bus, drive);
    }
    else {
        printf("Reading from ATAPI drives unsupported!\n");
    }
}

void ata_write_sectors(uint32_t buf, const uint64_t lba,
                       const uint32_t n, ata_bus_t *bus,
                       const ata_drvtype_t drive)
{
    uint8_t d = (drive == ATA_MASTER) ? ATA_DRV_MASTER : ATA_DRV_SLAVE;

    if (bus->drives[d].type == ATA_TYPE_ATA) {
        write_ata_sectors(buf, lba, n, bus, drive);
    }
    else {
        printf("Writing to ATAPI drive unsupported!\n");
    }
}

void init_ata()
{
    printf("Initializing ATA drives...\n");

    init_drives();
    /* register_interrupt_callback(0x20, &ata_primary_irq); */
    /* register_interrupt_callback(0x21, &ata_primary_irq); */
    /* register_interrupt_callback(0x22, &ata_primary_irq); */
    /* register_interrupt_callback(0x23, &ata_primary_irq); */
    /* register_interrupt_callback(0x24, &ata_primary_irq); */
    /* register_interrupt_callback(0x25, &ata_primary_irq); */
    /* register_interrupt_callback(0x26, &ata_primary_irq); */
    /* register_interrupt_callback(0x27, &ata_primary_irq); */
    /* register_interrupt_callback(0x28, &ata_primary_irq); */
    /* register_interrupt_callback(0x29, &ata_primary_irq); */
    /* register_interrupt_callback(0x2A, &ata_primary_irq); */
    /* register_interrupt_callback(0x2B, &ata_primary_irq); */
    /* register_interrupt_callback(0x2C, &ata_primary_irq); */
    register_interrupt_callback(0x20 + buses[ATA_BUS_PRI].irq, &ata_primary_irq);
    register_interrupt_callback(0x20 + buses[ATA_BUS_SEC].irq, &ata_secondary_irq);

    uint32_t *out = kmalloc(MEM_DMA, 256);
    if (!out) {
        PANIC("Unable to allocate contiguous DMA memory for test transfer!");
    }

    uint64_t lba = 0x2;

    for (uint32_t i = 0; i < 64; ++i) {
        out[i] = 0xDEADC0DE;
    }
    
    uint32_t out_phys = get_physical((uint32_t)out);
    ata_write_sectors(out_phys, lba, 1, &buses[ATA_BUS_PRI], ATA_MASTER);

    uint32_t *buf = kmalloc(MEM_DMA, 256);
    if (!out) {
        PANIC("Unable to allocate contiguous DMA memory for test transfer 2!");
    }

    uint32_t phys = get_physical((uint32_t)buf);
    ata_read_sectors(phys, lba, 1, &buses[ATA_BUS_PRI], ATA_MASTER);

    printf("Read data from block 1:\n");
    for (uint32_t i = 0; i < 256; ++i) {
        printf(" %x\n", buf[i]);
    }

    while(1);
}
