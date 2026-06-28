#include "components.h"
#include <stdio.h>

static const char* _pe_cv_guid_to_string(const PEGUID* guid) {
    static char buffer[37]; // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" + NUL

    snprintf(buffer, sizeof(buffer),
             "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid->data1,
             guid->data2, guid->data3, guid->data4[0], guid->data4[1],
             guid->data4[2], guid->data4[3], guid->data4[4], guid->data4[5],
             guid->data4[6], guid->data4[7]);

    return buffer;
}

const RDKBObject* pe_vb_components_find(RDContext* ctx, const PEGUID* guid) {
    if(!guid) return NULL;

    const RDKBObject* components = rd_kb_load(ctx, "pe/vb/components");
    if(!components) return NULL;

    const char* guid_str = _pe_cv_guid_to_string(guid);
    return rd_kbobject_get_table(components, guid_str);
}
