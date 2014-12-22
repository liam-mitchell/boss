#include "modules.h"

#include "fs.h"
#include "mboot.h"
#include "mm.h"
#include "terminal.h"

void init_modules(multiboot_info_t *mboot)
{
    module_t *mod = (module_t *)map_physical(mboot->mods_addr);
    uint32_t mod_start = map_physical(mod->mod_start);
    init_fs(mod_start);
}
