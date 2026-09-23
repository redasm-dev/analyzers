#include "load.h"
#include <redasm/redasm.h>

/*
 * This plugin can be configured using core settings:
 *
 *   [pdb]
 *   servers = [
 *       "https://msdl.microsoft.com/download/symbols",
 *       "https://your-internal-symbol-server/symbols",
 *   ]
 */

static void pdb_module_load(void) { rd_register_analyzer(&PDB_DEBUGINFO); }

RD_MODULE_EXPORT = {
    .api_version = RD_API_VERSION,
    .load = pdb_module_load,
};
