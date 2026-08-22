#include "rtti.h"
#include "msvc/common.h"
#include "msvc/rtti_types.h"
#include <inttypes.h>
#include <string.h>

static const char*
_msvc_rtti_extract_classtag(RDReader* r, RDAddress locator_addr,
                            const RTTICompleteObjectLocator* objlocator) {
    bool is64 = objlocator->signature == MSVC_RTTI_SIGNATURE_V1;

    RDAddress typedescriptor_va =
        is64 ? (locator_addr - objlocator->pSelf) + objlocator->pTypeDescriptor
             : objlocator->pTypeDescriptor;

    return msvc_rtti_classtag_from_typedescriptor(r, typedescriptor_va, is64);
}

static bool _msvc_rtti_walk_baseclassarray(
    RDContext* ctx, RDReader* r,
    const RTTIClassHierarchyDescriptor* classdescriptor, RDAddress imagebase,
    const char* classtag) {
    RDAddress baseclassarray_va = imagebase + classdescriptor->pBaseClassArray;

    for(u32 i = 0; i < classdescriptor->numBaseClasses; i++) {
        RDAddress entry_addr = baseclassarray_va + (i * sizeof(u32));

        u32 bcd_addr_val;
        rd_reader_seek(r, entry_addr);
        if(!rd_reader_read_le32(r, &bcd_addr_val)) return false;

        RDAddress bcd_va = imagebase + bcd_addr_val;
        if(!msvc_rtti_segment_ok(ctx, bcd_va)) return false;

        rd_library_type(ctx, entry_addr, "u32", 0, RD_TYPE_PTR);

        rd_reader_seek(r, bcd_va);
        RTTIBaseClassDescriptor basedescriptor;
        if(!msvc_rtti_read_baseclassdescriptor(r, &basedescriptor))
            return false;

        RDAddress typedescriptor_va =
            imagebase + basedescriptor.pTypeDescriptor;

        if(!msvc_rtti_segment_ok(ctx, typedescriptor_va)) return false;

        if(basedescriptor.attributes & MSVC_RTTI_BCD_HASCHD) {
            RDAddress classdescriptor_va =
                imagebase + basedescriptor.pClassDescriptor;

            if(!msvc_rtti_segment_ok(ctx, classdescriptor_va)) return false;
        }

        if(basedescriptor.numContainedBases > MSVC_RTTI_MAX_BASE_CLASSES)
            return false;

        rd_library_type(ctx, bcd_va, "RTTI_BaseClassDescriptor", 0,
                        RD_TYPE_NONE);

        if(classtag) {
            rd_library_name(ctx, bcd_va,
                            rd_format("%s::__base_class", classtag));
        }
    }

    return true;
}

static bool _msvc_rtti_check_completeobjectlocator_v0(
    RDContext* ctx, RDReader* r, const RTTICompleteObjectLocator* objlocator) {
    if(!msvc_rtti_segment_ok(ctx, objlocator->pTypeDescriptor)) return false;
    if(!msvc_rtti_segment_ok(ctx, objlocator->pClassDescriptor)) return false;

    rd_reader_seek(r, objlocator->pTypeDescriptor);

    RTTITypeDescriptor32 typedescriptor;
    if(!msvc_rtti_read_typedescriptor32(r, &typedescriptor)) return false;

    RDAddress typedescriptor_name = (RDAddress)rd_reader_tell(r);

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!msvc_rtti_is_typedescriptor_valid(name, n)) return false;

    rd_reader_seek(r, objlocator->pClassDescriptor);

    RTTIClassHierarchyDescriptor classdescriptor;
    if(!msvc_rtti_read_classhierarchydescriptor(r, &classdescriptor))
        return false;

    if(classdescriptor.signature != 0) return false;
    if(classdescriptor.numBaseClasses > MSVC_RTTI_MAX_BASE_CLASSES)
        return false;

    if(!msvc_rtti_segment_fits(ctx, classdescriptor.pBaseClassArray,
                               (usize)classdescriptor.numBaseClasses *
                                   sizeof(u32)))
        return false;

    char* classtag = rd_strdup(_msvc_rtti_extract_classtag(r, 0, objlocator));

    if(!_msvc_rtti_walk_baseclassarray(ctx, r, &classdescriptor, 0, classtag)) {
        rd_free(classtag);
        return false;
    }

    rd_library_type(ctx, typedescriptor_name, "char", n + 1, RD_TYPE_NONE);

    rd_library_type(ctx, objlocator->pClassDescriptor,
                    "RTTI_ClassHierarchyDescriptor", 0, RD_TYPE_NONE);

    rd_library_type(ctx, objlocator->pTypeDescriptor, "RTTI_TypeDescriptor", 0,
                    RD_TYPE_NONE);

    if(rd_reader_has_error(r)) {
        rd_free(classtag);
        return false;
    }

    if(classtag) {
        rd_library_name(ctx, objlocator->pClassDescriptor,
                        rd_format("%s::__rtti_class_hierarchy", classtag));
        rd_library_name(ctx, objlocator->pTypeDescriptor,
                        rd_format("%s::__rtti_type_descriptor", classtag));
    }

    rd_free(classtag);
    return true;
}

static bool _msvc_rtti_check_completeobjectlocator_v1(
    RDContext* ctx, RDReader* r, RDAddress locator_addr,
    const RTTICompleteObjectLocator* objlocator) {

    RDAddress imagebase = locator_addr - objlocator->pSelf;
    RDAddress td_va = imagebase + objlocator->pTypeDescriptor;
    RDAddress cd_va = imagebase + objlocator->pClassDescriptor;

    if(!msvc_rtti_segment_ok(ctx, td_va)) return false;
    if(!msvc_rtti_segment_ok(ctx, cd_va)) return false;

    rd_reader_seek(r, td_va);
    RTTITypeDescriptor64 typedescriptor;
    if(!msvc_rtti_read_typedescriptor64(r, &typedescriptor)) return false;

    RDAddress typedescriptor_name = (RDAddress)rd_reader_tell(r);

    usize n;
    const char* name = rd_reader_read_str(r, &n);
    if(!msvc_rtti_is_typedescriptor_valid(name, n)) return false;

    rd_reader_seek(r, cd_va);
    RTTIClassHierarchyDescriptor classdescriptor;
    if(!msvc_rtti_read_classhierarchydescriptor(r, &classdescriptor))
        return false;

    if(classdescriptor.signature != 0) return false;
    if(classdescriptor.numBaseClasses > MSVC_RTTI_MAX_BASE_CLASSES)
        return false;

    u64 bca_va = imagebase + classdescriptor.pBaseClassArray;
    if(!msvc_rtti_segment_fits(
           ctx, bca_va, (usize)classdescriptor.numBaseClasses * sizeof(u32)))
        return false;

    char* classtag =
        rd_strdup(_msvc_rtti_extract_classtag(r, locator_addr, objlocator));

    if(!_msvc_rtti_walk_baseclassarray(ctx, r, &classdescriptor, imagebase,
                                       classtag)) {
        rd_free(classtag);
        return false;
    }

    rd_library_type(ctx, typedescriptor_name, "char", n + 1, RD_TYPE_NONE);
    rd_library_type(ctx, cd_va, "RTTI_ClassHierarchyDescriptor", 0,
                    RD_TYPE_NONE);
    rd_library_type(ctx, td_va, "RTTI_TypeDescriptor", 0, RD_TYPE_NONE);

    if(rd_reader_has_error(r)) {
        rd_free(classtag);
        return false;
    }

    if(classtag) {
        rd_library_name(ctx, cd_va,
                        rd_format("%s::__rtti_class_hierarchy", classtag));
        rd_library_name(ctx, td_va,
                        rd_format("%s::__rtti_type_descriptor", classtag));
    }

    rd_free(classtag);
    return true;
}

static void _msvc_rtti_process_vtable(RDContext* ctx, RDReader* r,
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
               !strcmp(rd_typedef_name(t.def), objlocator_type)) {
                name = rd_format("%s::__obj_locator", classtag_ptr);
                rd_library_name(ctx, vtable_addr, name);
                rd_library_type(ctx, vtable_addr, rd_integral_from_size(stride),
                                0, RD_TYPE_PTR);

                rd_add_xref(ctx, vtable_addr, vtable_entryaddr, RD_DR_ADDRESS);
            }

            break;
        }

        name = rd_format("%s::vfunc_%" PRIX64, classtag_ptr, vtable_entryaddr);
        rd_set_function(ctx, vtable_entryaddr);
        rd_auto_name(ctx, vtable_entryaddr, name);

        // vtable entry
        name = rd_format("%s::__vtable_%" PRId32, classtag_ptr, index);
        rd_library_name(ctx, vtable_addr, name);
        rd_library_type(ctx, vtable_addr, rd_integral_from_size(stride), 0,
                        RD_TYPE_PTR);
        rd_add_xref(ctx, vtable_addr, vtable_entryaddr, RD_DR_ADDRESS);

        slot += stride;
        index++;
    }

    rd_free(classtag_ptr);
}

static void _msvc_rtti_find_vtables(RDContext* ctx, RDReader* r,
                                    int signature_type) {

    const char* objlocator_type = signature_type == MSVC_RTTI_SIGNATURE_V0
                                      ? "RTTI_CompleteObjectLocator32"
                                      : "RTTI_CompleteObjectLocator64";

    RDAddressSlice locators = rd_get_all_address_by_type(ctx, objlocator_type);
    if(rd_slice_is_empty(locators)) return;

    usize stride =
        signature_type == MSVC_RTTI_SIGNATURE_V0 ? sizeof(u32) : sizeof(u64);

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

            if(signature_type == MSVC_RTTI_SIGNATURE_V0) {
                u32 v;
                ok = rd_reader_read_le32(r, &v);
                value = (RDAddress)v;
            }
            else {
                u64 v;
                ok = rd_reader_read_le64(r, &v);
                value = (RDAddress)v;
            }

            if(ok && msvc_rtti_addressslice_contains(locators, value)) {

                RDAddress vtable_addr = slot + stride;

                // re-read the locator to recover its TypeDescriptor/classtag
                rd_reader_seek(r, value);
                RTTICompleteObjectLocator objlocator;
                if(msvc_rtti_read_completeobjectlocator(r, &objlocator)) {
                    const char* classtag =
                        _msvc_rtti_extract_classtag(r, value, &objlocator);

                    if(classtag) {
                        _msvc_rtti_process_vtable(ctx, r, vtable_addr, classtag,
                                                  stride, objlocator_type);
                    }
                }
            }

            slot += stride;
        }
    }
}

static void _msvc_rtti_find_objlocators(RDContext* ctx, RDReader* r) {
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
            bool ok = msvc_rtti_read_completeobjectlocator(r, &objlocator);

            if(ok) {
                next = (RDAddress)rd_reader_tell(r);

                if(objlocator.signature == MSVC_RTTI_SIGNATURE_V0) {
                    ok = _msvc_rtti_check_completeobjectlocator_v0(ctx, r,
                                                                   &objlocator);

                    if(ok) {
                        rd_library_type(ctx, addr,
                                        "RTTI_CompleteObjectLocator32", 0,
                                        RD_TYPE_NONE);
                    }
                }
                else { // if(objlocator.signature == MSVC_RTTI_SIGNATURE_V1)
                    ok = _msvc_rtti_check_completeobjectlocator_v1(ctx, r, addr,
                                                                   &objlocator);

                    if(ok) {
                        rd_library_type(ctx, addr,
                                        "RTTI_CompleteObjectLocator64", 0,
                                        RD_TYPE_NONE);
                    }
                }
            }

            if(ok) {
                char* classtag = rd_strdup(
                    _msvc_rtti_extract_classtag(r, addr, &objlocator));

                if(classtag) {
                    rd_library_name(
                        ctx, addr,
                        rd_format("%s::__rtti_obj_locator", classtag));
                }

                rd_free(classtag);

                addr = next;
            }
            else
                addr += sizeof(u32);
        }
    }
}

static void msvc_rtti_execute(RDContext* ctx) {
    rd_kb_load(ctx, "compiler/msvc/rtti");

    RDReader* r = rd_get_reader(ctx);
    _msvc_rtti_find_objlocators(ctx, r);
    _msvc_rtti_find_vtables(ctx, r, MSVC_RTTI_SIGNATURE_V0);
    _msvc_rtti_find_vtables(ctx, r, MSVC_RTTI_SIGNATURE_V1);
}

const RDAnalyzerPlugin MSVC_RTTI = {
    .level = RD_API_LEVEL,
    .id = "compiler_msvc_rtti",
    .name = "Decode MSVC Runtime Type Information (RTTI)",
    .flags = RD_AF_RUNONCE | RD_AF_EXPERIMENTAL,
    .order = 1000,
    .execute = msvc_rtti_execute,
};
