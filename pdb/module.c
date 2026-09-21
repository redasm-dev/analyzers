#include "load.h"
#include <redasm/redasm.h>

static void pdb_module_load(void) { rd_register_analyzer(&PDB_DEBUGINFO); }

RD_MODULE_EXPORT = {
    .api_version = RD_API_VERSION,
    .load = pdb_module_load,
};
