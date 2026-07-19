#pragma once

#include "vb/format.h"

typedef struct PEVBComponent {
    const char* name;
    const char* guid_str;
    const char* const* events;
} PEVBComponent;

const RDKBObject* vb_components_find(RDContext* ctx, const VBGUID* guid);
