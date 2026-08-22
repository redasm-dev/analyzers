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

#define MSVC_EH_FUNCINFO_MAGIC_BASE 0x19930520u
#define MSVC_EH_FUNCINFO_MAGIC_MASK 0x1FFFFFFFu // magicNumber is a 29-bit field
#define MSVC_EH_FUNCINFO_MAGIC_SPAN 0xFu // accepted compiler-version range

#define MSVC_EH_MAX_UNWIND_STATES 4096
#define MSVC_EH_MAX_TRY_BLOCKS 256
#define MSVC_EH_MAX_HANDLERS_PER_TRY 64

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

typedef struct EHFuncInfo {
    u32 magicAndBbtFlags;
    u32 maxState;
    u32 pUnwindMap;
    u32 nTryBlocks;
    u32 pTryBlockMap;
    u32 nIPMapEntries;
    u32 pIPtoStateMap;
} EHFuncInfo;

typedef struct EHUnwindMapEntry {
    i32 toState;
    u32 action;
} EHUnwindMapEntry;

typedef struct EHTryBlockMapEntry {
    i32 tryLow;
    i32 tryHigh;
    i32 catchHigh;
    u32 nCatches;
    u32 pHandlerArray;
} EHTryBlockMapEntry;

typedef struct EHHandlerType {
    u32 adjectives;
    u32 pType;
    i32 dispCatchObj;
    u32 addressOfHandler;
} EHHandlerType;

bool msvc_eh_read_catchabletype(RDReader* r, EHCatchableType* v);
bool msvc_eh_read_catchabletypearrayheader(RDReader* r,
                                           EHCatchableTypeArrayHeader* v);
bool msvc_eh_read_throwinfo(RDReader* r, EHThrowInfo* v);

bool msvc_eh_read_funcinfo(RDReader* r, EHFuncInfo* v);
bool msvc_eh_read_unwindmapentry(RDReader* r, EHUnwindMapEntry* v);
bool msvc_eh_read_tryblockmapentry(RDReader* r, EHTryBlockMapEntry* v);
bool msvc_eh_read_handlertype(RDReader* r, EHHandlerType* v);
