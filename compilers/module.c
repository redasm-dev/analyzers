#include "msvc/eh.h"
#include "msvc/rtti.h"
#include "vb/decompiler.h"
#include <redasm/redasm.h>

static void compilers_module_load(void) {
    rd_register_analyzer(&MSVC_RTTI);
    rd_register_analyzer(&MSVC_EH);
    rd_register_analyzer(&VB_DECOMPILER);
}

RD_MODULE_EXPORT = {
    .api_version = RD_API_VERSION,
    .load = compilers_module_load,
};
