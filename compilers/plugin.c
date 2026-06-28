#include "rtti/msvc/msvc.h"
#include "vb/decompiler.h"
#include <redasm/redasm.h>

void rd_plugin_create(void) {
    rd_register_analyzer(&RTTI_MSVC);
    rd_register_analyzer(&VB_DECOMPILER);
}

const char* rd_plugin_version(void) { return "1.0"; }
