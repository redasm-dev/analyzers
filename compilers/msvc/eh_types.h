#pragma once

#include <redasm/redasm.h>

#define MSVC_EH_MAX_CATCHABLE_TYPES 64

#define MSVC_EH_PROP_ISSIMPLETYPE 0x1
#define MSVC_EH_PROP_BYREFERENCE 0x2
#define MSVC_EH_PROP_HASVIRTBASE 0x4
#define MSVC_EH_PROP_ISWINRT 0x8
#define MSVC_EH_PROP_ISBADALLOC 0x10
#define MSVC_EH_PROP_MASK 0x1F // conservative upper bound for validation

#define MSVC_EH_ATTR_CONST 0x1
#define MSVC_EH_ATTR_VOLATILE 0x2
#define MSVC_EH_ATTR_UNALIGNED 0x4 // ARM only
#define MSVC_EH_ATTR_WINRT 0x8
#define MSVC_EH_ATTR_MASK 0xF // conservative upper bound for validation

typedef struct EHCatchableType {
    u32 properties;
    u32 pType;
    i32 where_mdisp;
    i32 where_pdisp;
    i32 where_vdisp;
    u32 sizeOrOffset;
    u32 copyFunction;
} EHCatchableType;

typedef struct EHCatchableTypeArrayHeader {
    u32 nCatchableTypes;
    // followed by nCatchableTypes x u32 entries (absolute VA for V0,
    // image-relative offset for V1)
    // read separately, not part of the fixed-size header.
} EHCatchableTypeArrayHeader;

typedef struct EHThrowInfo {
    u32 attributes;
    u32 pmfnUnwind;
    u32 pForwardCompat;
    u32 pCatchableTypeArray;
} EHThrowInfo;

bool msvc_eh_read_catchabletype(RDReader* r, EHCatchableType* v);
bool msvc_eh_read_catchabletypearrayheader(RDReader* r,
                                           EHCatchableTypeArrayHeader* v);
bool msvc_eh_read_throwinfo(RDReader* r, EHThrowInfo* v);
