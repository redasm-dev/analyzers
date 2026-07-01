#include "msvc.h"
#include "rtti/msvc/types.h"
#include <inttypes.h>
#include <string.h>

#define RTTI_MSVC_PREFIX ".?AV"
#define RTTI_MSVC_SUFFIX "@@"

static bool _rtti_msvc_segment_ok(RDContext* ctx, RDAddress address) {
    const RDSegment* seg = rd_find_segment(ctx, address);
    return seg && (seg->perm & RD_SP_R) && !(seg->perm & RD_SP_X);
}

static bool _rtti_msvc_segment_fits(RDContext* ctx, RDAddress address,
                                    usize n) {
    const RDSegment* seg = rd_find_segment(ctx, address);
    if(!seg || (!(seg->perm & RD_SP_R)) || seg->perm & RD_SP_X) return false;

    // also guards overflow if size is bounded upstream
    return (address + n) <= seg->end_address;
}

static bool _rtti_msvc_is_typedescriptor_valid(const char* s, usize n) {
    if(!s || n < sizeof(RTTI_MSVC_PREFIX) - 1) return false;

    if(strncmp(s, RTTI_MSVC_PREFIX, sizeof(RTTI_MSVC_PREFIX) - 1) != 0)
        return false;

    const char* end_s = s + (n - sizeof(RTTI_MSVC_SUFFIX) + 1);

    if(strncmp(end_s, RTTI_MSVC_SUFFIX, sizeof(RTTI_MSVC_SUFFIX) - 1) != 0)
        return false;

    return true;
}

static bool _rtti_msvc_walk_baseclassarray(
    RDContext* ctx, RDReader* r,
    const RTTIClassHierarchyDescriptor* classdescriptor, RDAddress imagebase) {
    RDAddress baseclassarray_va = imagebase + classdescriptor->pBaseClassArray;

    for(u32 i = 0; i < classdescriptor->numBaseClasses; i++) {
        RDAddress entry_addr = baseclassarray_va + (i * sizeof(u32));

        u32 bcd_addr_val;
        rd_reader_seek(r, entry_addr);
        if(!rd_reader_read_le32(r, &bcd_addr_val)) return false;

        RDAddress bcd_va = imagebase + bcd_addr_val;
        if(!_rtti_msvc_segment_ok(ctx, bcd_va)) return false;

        rd_reader_seek(r, bcd_va);
        RTTIBaseClassDescriptor basedescriptor;
        if(!rtti_msvc_read_baseclassdescriptor(r, &basedescriptor))
            return false;

        RDAddress typedescriptor_va =
            imagebase + basedescriptor.pTypeDescriptor;

        if(!_rtti_msvc_segment_ok(ctx, typedescriptor_va)) return false;

        if(basedescriptor.attributes & RTTI_MSVC_BCD_HASCHD) {
            RDAddress classdescriptor_va =
                imagebase + basedescriptor.pClassDescriptor;

            if(!_rtti_msvc_segment_ok(ctx, classdescriptor_va)) return false;
        }

        if(basedescriptor.numContainedBases > RTTI_MSVC_MAX_BASE_CLASSES)
            return false;

        rd_library_type(ctx, bcd_va, "RTTI_BaseClassDescriptor", 0,
                        RD_TYPE_NONE);
    }

    return true;
}

static bool _rtti_msvc_check_completeobjectlocator_v0(
    RDContext* ctx, RDReader* r, const RTTICompleteObjectLocator* objlocator) {
    if(!_rtti_msvc_segment_ok(ctx, objlocator->pTypeDescriptor)) return false;
    if(!_rtti_msvc_segment_ok(ctx, objlocator->pClassDescriptor)) return false;

    rd_reader_seek(r, objlocator->pTypeDescriptor);

    RTTITypeDescriptor32 typedescriptor;
    if(!rtti_msvc_read_typedescriptor32(r, &typedescriptor)) return false;

    RDAddress typedescriptor_name = (RDAddress)rd_reader_tell(r);

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!_rtti_msvc_is_typedescriptor_valid(name, n)) return false;

    rd_reader_seek(r, objlocator->pClassDescriptor);

    RTTIClassHierarchyDescriptor classdescriptor;
    if(!rtti_msvc_read_classhierarchydescriptor(r, &classdescriptor))
        return false;

    if(classdescriptor.signature != 0) return false;
    if(classdescriptor.numBaseClasses > RTTI_MSVC_MAX_BASE_CLASSES)
        return false;

    if(!_rtti_msvc_segment_fits(ctx, classdescriptor.pBaseClassArray,
                                (usize)classdescriptor.numBaseClasses *
                                    sizeof(u32)))
        return false;

    if(!_rtti_msvc_walk_baseclassarray(ctx, r, &classdescriptor, 0))
        return false;

    rd_library_type(ctx, typedescriptor_name, "char", n + 1, RD_TYPE_NONE);

    rd_library_type(ctx, objlocator->pClassDescriptor,
                    "RTTI_ClassHierarchyDescriptor", 0, RD_TYPE_NONE);

    rd_library_type(ctx, objlocator->pTypeDescriptor, "RTTI_TypeDescriptor", 0,
                    RD_TYPE_NONE);

    return !rd_reader_has_error(r);
}

static bool _rtti_msvc_check_completeobjectlocator_v1(
    RDContext* ctx, RDReader* r, RDAddress locator_addr,
    const RTTICompleteObjectLocator* objlocator) {

    RDAddress imagebase = locator_addr - objlocator->pSelf;
    RDAddress td_va = imagebase + objlocator->pTypeDescriptor;
    RDAddress cd_va = imagebase + objlocator->pClassDescriptor;

    if(!_rtti_msvc_segment_ok(ctx, td_va)) return false;
    if(!_rtti_msvc_segment_ok(ctx, cd_va)) return false;

    rd_reader_seek(r, td_va);
    RTTITypeDescriptor64 typedescriptor;
    if(!rtti_msvc_read_typedescriptor64(r, &typedescriptor)) return false;

    RDAddress typedescriptor_name = (RDAddress)rd_reader_tell(r);

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!_rtti_msvc_is_typedescriptor_valid(name, n)) return false;

    rd_reader_seek(r, cd_va);
    RTTIClassHierarchyDescriptor classdescriptor;
    if(!rtti_msvc_read_classhierarchydescriptor(r, &classdescriptor))
        return false;

    if(classdescriptor.signature != 0) return false;
    if(classdescriptor.numBaseClasses > RTTI_MSVC_MAX_BASE_CLASSES)
        return false;

    u64 bca_va = imagebase + classdescriptor.pBaseClassArray;
    if(!_rtti_msvc_segment_fits(
           ctx, bca_va, (usize)classdescriptor.numBaseClasses * sizeof(u32)))
        return false;

    if(!_rtti_msvc_walk_baseclassarray(ctx, r, &classdescriptor, imagebase))
        return false;

    rd_library_type(ctx, typedescriptor_name, "char", n + 1, RD_TYPE_NONE);
    rd_library_type(ctx, cd_va, "RTTI_ClassHierarchyDescriptor", 0,
                    RD_TYPE_NONE);
    rd_library_type(ctx, td_va, "RTTI_TypeDescriptor64", 0, RD_TYPE_NONE);

    return !rd_reader_has_error(r);
}

static void _rtti_msvc_process_vtable(RDContext* ctx, RDReader* r,
                                      RDAddress vtable_addr,
                                      const char* classtag, usize stride,
                                      const char* objlocator_type) {
    RDAddress slot = vtable_addr;
    u32 index = 0;

    char* classtag_ptr = rd_strdup(classtag);

    while(true) {
        rd_reader_seek(r, slot);
        RDAddress vtable_addr = (RDAddress)rd_reader_tell(r);
        RDAddress vtable_entryaddr;

        if(stride == sizeof(u32)) {
            u32 addr;
            if(!rd_reader_read_le32(r, &addr)) break;
            vtable_entryaddr = (RDAddress)addr;
        }
        else {
            u64 addr;
            if(!rd_reader_read_le64(r, &addr)) break;
            vtable_entryaddr = (RDAddress)addr;
        }

        const RDSegment* seg = rd_find_segment(ctx, vtable_entryaddr);
        if(!seg) break;

        const char* name = NULL;

        if(!(seg->perm & RD_SP_X)) {
            RDType t;

            if(rd_get_type(ctx, vtable_entryaddr, &t) &&
               !strcmp(t.name, objlocator_type)) {
                name = rd_format("%s::obj_locator", classtag_ptr);
                rd_library_name(ctx, vtable_addr, name);
                rd_library_type(ctx, vtable_addr, rd_integral_from_size(stride),
                                0, RD_TYPE_NONE);
            }

            break;
        }

        name = rd_format("%s::vfunc_%" PRIx64, classtag_ptr, vtable_entryaddr);
        rd_auto_function(ctx, vtable_entryaddr, name);

        // vtable entry
        name = rd_format("%s::vtable_%" PRId32, classtag_ptr, index);
        rd_library_name(ctx, vtable_addr, name);
        rd_library_type(ctx, vtable_addr, rd_integral_from_size(stride), 0,
                        RD_TYPE_NONE);

        slot += stride;
        index++;
    }

    rd_free(classtag_ptr);
}

static bool _rtti_msvc_addressvect_contains(RDAddressSlice locators,
                                            RDAddress address) {
    const RDAddress* addr;
    rd_slice_each(addr, locators) {
        if(*addr == address) return true;
    }

    return false;
}

static const char*
_rtti_msvc_extract_classtag(RDReader* r, RDAddress locator_addr,
                            const RTTICompleteObjectLocator* objlocator) {

    RDAddress typedescriptor_va;

    if(objlocator->signature == RTTI_MSVC_SIGNATURE_V0) {
        typedescriptor_va = objlocator->pTypeDescriptor;

        rd_reader_seek(r, typedescriptor_va);
        RTTITypeDescriptor32 typedescriptor;
        if(!rtti_msvc_read_typedescriptor32(r, &typedescriptor)) return NULL;
    }
    else { // RTTI_MSVC_SIGNATURE_V1
        RDAddress imagebase = locator_addr - objlocator->pSelf;
        typedescriptor_va = imagebase + objlocator->pTypeDescriptor;

        rd_reader_seek(r, typedescriptor_va);
        RTTITypeDescriptor64 typedescriptor;
        if(!rtti_msvc_read_typedescriptor64(r, &typedescriptor)) return NULL;
    }

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!name || n < 6) return NULL;
    return rd_format("%.*s", (int)(n - 6), name + 4);
}

static void _rtti_msvc_find_vtables(RDContext* ctx, RDReader* r,
                                    int signature_type) {

    const char* objlocator_type = signature_type == RTTI_MSVC_SIGNATURE_V0
                                      ? "RTTI_CompleteObjectLocator32"
                                      : "RTTI_CompleteObjectLocator64";

    RDAddressSlice locators = rd_get_all_address_by_type(ctx, objlocator_type);
    if(rd_slice_is_empty(locators)) return;

    usize stride =
        signature_type == RTTI_MSVC_SIGNATURE_V0 ? sizeof(u32) : sizeof(u64);

    RDSegmentSlice segments = rd_get_all_segments(ctx);
    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if((!(seg->perm & RD_SP_R)) || seg->perm & RD_SP_X) continue;

        RDAddress slot = seg->start_address;

        while(slot + stride <= seg->end_address) {
            rd_reader_seek(r, slot);
            RDAddress value;
            bool ok;

            if(signature_type == RTTI_MSVC_SIGNATURE_V0) {
                u32 v;
                ok = rd_reader_read_le32(r, &v);
                value = (RDAddress)v;
            }
            else {
                u64 v;
                ok = rd_reader_read_le64(r, &v);
                value = (RDAddress)v;
            }

            if(ok && _rtti_msvc_addressvect_contains(locators, value)) {

                RDAddress vtable_addr = slot + stride;

                // re-read the locator to recover its TypeDescriptor/classtag
                rd_reader_seek(r, value);
                RTTICompleteObjectLocator objlocator;
                if(rtti_msvc_read_completeobjectlocator(r, &objlocator)) {
                    const char* classtag =
                        _rtti_msvc_extract_classtag(r, value, &objlocator);

                    if(classtag) {
                        _rtti_msvc_process_vtable(ctx, r, vtable_addr, classtag,
                                                  stride, objlocator_type);
                    }
                }
            }

            slot += stride;
        }
    }
}

static void _rtti_msvc_find_objlocators(RDContext* ctx, RDReader* r) {
    RDSegmentSlice segments = rd_get_all_segments(ctx);

    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if((!(seg->perm & RD_SP_R)) || seg->perm & RD_SP_X) continue;

        RDAddress addr = seg->start_address;

        while(addr < seg->end_address) {
            rd_reader_seek(r, addr);

            RDAddress next = 0;
            RTTICompleteObjectLocator objlocator;
            bool ok = rtti_msvc_read_completeobjectlocator(r, &objlocator);

            if(ok) {
                next = (RDAddress)rd_reader_tell(r);

                if(objlocator.signature == RTTI_MSVC_SIGNATURE_V0) {
                    ok = _rtti_msvc_check_completeobjectlocator_v0(ctx, r,
                                                                   &objlocator);

                    if(ok) {
                        rd_library_type(ctx, addr,
                                        "RTTI_CompleteObjectLocator32", 0,
                                        RD_TYPE_NONE);
                    }
                }
                else { // if(objlocator.signature == RTTI_MSVC_SIGNATURE_V1)
                    ok = _rtti_msvc_check_completeobjectlocator_v1(ctx, r, addr,
                                                                   &objlocator);

                    if(ok) {
                        rd_library_type(ctx, addr,
                                        "RTTI_CompleteObjectLocator64", 0,
                                        RD_TYPE_NONE);
                    }
                }
            }

            if(ok)
                addr = next;
            else
                addr += sizeof(u32);
        }
    }
}

static void rtti_msvc_execute(RDContext* ctx) {
    rd_kb_load(ctx, "compiler/msvc/rtti");

    RDReader* r = rd_get_reader(ctx);
    _rtti_msvc_find_objlocators(ctx, r);
    _rtti_msvc_find_vtables(ctx, r, RTTI_MSVC_SIGNATURE_V0);
    _rtti_msvc_find_vtables(ctx, r, RTTI_MSVC_SIGNATURE_V1);
}

const RDAnalyzerPlugin RTTI_MSVC = {
    .level = RD_API_LEVEL,
    .id = "compiler_rtti_msvc",
    .name = "Decode MSVC RTTI",
    .flags = RD_AF_SELECTED | RD_AF_RUNONCE,
    .order = 1000,
    .execute = rtti_msvc_execute,
};
