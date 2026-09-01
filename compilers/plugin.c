#include "msvc/eh.h"
#include "msvc/rtti.h"
#include "vb/decompiler.h"
#include <redasm/redasm.h>

void rd_plugin_create(void) {
    rd_register_analyzer(&MSVC_RTTI);
    rd_register_analyzer(&MSVC_EH);
    rd_register_analyzer(&VB_DECOMPILER);
}
