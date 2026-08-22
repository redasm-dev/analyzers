#include "common.h"
#include "msvc/rtti_types.h"
#include <string.h>

#define MSVC_RTTI_PREFIX ".?AV"
#define MSVC_RTTI_SUFFIX "@@"

const char* msvc_rtti_classtag_from_typedescriptor(RDReader* r, RDAddress td_va,
                                                   bool is64) {
    rd_reader_seek(r, td_va);

    if(is64) {
        RTTITypeDescriptor64 td;
        if(!msvc_rtti_read_typedescriptor64(r, &td)) return NULL;
    }
    else {
        RTTITypeDescriptor32 td;
        if(!msvc_rtti_read_typedescriptor32(r, &td)) return NULL;
    }

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!msvc_rtti_is_typedescriptor_valid(name, n) || n < 6) return NULL;
    return rd_format("%.*s", (int)(n - 6), name + 4);
}

bool msvc_rtti_addressslice_contains(RDAddressSlice locators,
                                     RDAddress address) {
    const RDAddress* addr;
    rd_slice_each(addr, locators) {
        if(*addr == address) return true;
    }

    return false;
}

bool msvc_rtti_segment_ok(RDContext* ctx, RDAddress address) {
    const RDSegment* seg = rd_find_segment(ctx, address);
    return seg && (seg->perm & RD_SP_R) && !(seg->perm & RD_SP_X);
}

bool msvc_rtti_segment_exec_ok(RDContext* ctx, RDAddress address) {
    const RDSegment* seg = rd_find_segment(ctx, address);
    return seg && (seg->perm & RD_SP_X);
}

bool msvc_rtti_segment_fits(RDContext* ctx, RDAddress address, usize n) {
    const RDSegment* seg = rd_find_segment(ctx, address);
    if(!seg || (!(seg->perm & RD_SP_R)) || seg->perm & RD_SP_X) return false;

    // also guards overflow if size is bounded upstream
    return (address + n) <= seg->end_address;
}

bool msvc_rtti_is_typedescriptor_valid(const char* s, usize n) {
    if(!s || n < sizeof(MSVC_RTTI_PREFIX) - 1) return false;

    if(strncmp(s, MSVC_RTTI_PREFIX, sizeof(MSVC_RTTI_PREFIX) - 1) != 0)
        return false;

    const char* end_s = s + (n - sizeof(MSVC_RTTI_SUFFIX) + 1);

    if(strncmp(end_s, MSVC_RTTI_SUFFIX, sizeof(MSVC_RTTI_SUFFIX) - 1) != 0)
        return false;

    return true;
}
