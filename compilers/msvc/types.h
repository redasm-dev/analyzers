#pragma once

#include <redasm/redasm.h>

#define RTTI_MSVC_SIGNATURE_V0 0
#define RTTI_MSVC_SIGNATURE_V1 1

#define RTTI_MSVC_MAX_BASE_CLASSES 256

#define RTTI_MSVC_BCD_HASCHD 0x40

typedef struct RTTICompleteObjectLocator {
    u32 signature;
    u32 offset;
    u32 cdOffset;
    u32 pTypeDescriptor;
    u32 pClassDescriptor;
    u32 pSelf;
} RTTICompleteObjectLocator;

typedef struct RTTIClassHierarchyDescriptor {
    u32 signature;
    u32 attributes;
    u32 numBaseClasses;
    u32 pBaseClassArray;
} RTTIClassHierarchyDescriptor;

typedef struct RTTIBaseClassDescriptor {
    u32 pTypeDescriptor;
    u32 numContainedBases;
    i32 where_mdisp;
    i32 where_pdisp;
    i32 where_vdisp;
    u32 attributes;
    u32 pClassDescriptor;
} RTTIBaseClassDescriptor;

typedef struct RTTITypeDescriptor32 {
    u32 pVFTable;
    u32 spare;
    // char name[];
} RTTITypeDescriptor32;

typedef struct RTTITypeDescriptor64 {
    u64 pVFTable;
    u64 spare;
    // char name[];
} RTTITypeDescriptor64;

bool rtti_msvc_read_completeobjectlocator(RDReader* r,
                                          RTTICompleteObjectLocator* v);
bool rtti_msvc_read_classhierarchydescriptor(RDReader* r,
                                             RTTIClassHierarchyDescriptor* v);
bool rtti_msvc_read_baseclassdescriptor(RDReader* r,
                                        RTTIBaseClassDescriptor* v);
bool rtti_msvc_read_typedescriptor32(RDReader* r, RTTITypeDescriptor32* v);
bool rtti_msvc_read_typedescriptor64(RDReader* r, RTTITypeDescriptor64* v);
