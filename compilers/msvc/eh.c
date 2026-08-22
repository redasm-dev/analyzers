#include "eh.h"
#include "msvc/common.h"
#include "msvc/eh_types.h"
#include "msvc/rtti_types.h"
#include <inttypes.h>

// Conservative plausibility bounds for candidate structures found while
// scanning raw, untrusted bytes.
// These are filters, not correctness guarantees.
#define MSVC_EH_MAX_DISP 0x00100000 // PMD fields: displacement in an object
#define MSVC_EH_MAX_SIZE 0x10000000 // sizeOrOffset: sizeof(object)

static bool _msvc_eh_catchabletype_plausible(RDContext* ctx,
                                             const EHCatchableType* ct,
                                             RDAddress imagebase) {
    if(ct->properties & ~(u32)MSVC_EH_PROP_MASK) return false;

    if(ct->where_mdisp < -MSVC_EH_MAX_DISP ||
       ct->where_mdisp > MSVC_EH_MAX_DISP)
        return false;
    if(ct->where_pdisp < -MSVC_EH_MAX_DISP ||
       ct->where_pdisp > MSVC_EH_MAX_DISP)
        return false;
    if(ct->where_vdisp < -MSVC_EH_MAX_DISP ||
       ct->where_vdisp > MSVC_EH_MAX_DISP)
        return false;

    if(ct->sizeOrOffset > MSVC_EH_MAX_SIZE) return false;

    // 0 is valid (e.g. trivially-copyable types have no copy ctor entry);
    // non-zero must land in code.
    if(ct->copyFunction &&
       !msvc_rtti_segment_exec_ok(ctx, imagebase + ct->copyFunction))
        return false;

    return true;
}

static void _msvc_eh_find_catchabletypes(RDContext* ctx, RDReader* r,
                                         RDAddress imagebase) {
    RDAddressSlice tds = rd_get_all_address_by_type(ctx, "RTTI_TypeDescriptor");
    if(rd_slice_is_empty(tds)) return;

    bool is64 = imagebase != 0;
    RDSegmentSlice segments = rd_get_all_segments(ctx);

    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if(!(seg->perm & RD_SP_R) || (seg->perm & RD_SP_X)) continue;

        RDAddress addr = seg->start_address;

        while(addr < seg->end_address) {
            rd_reader_seek(r, addr);

            EHCatchableType ct;
            bool ok = msvc_eh_read_catchabletype(r, &ct);
            RDAddress next = ok ? (RDAddress)rd_reader_tell(r) : 0;

            if(ok) {
                RDAddress ptype_va = imagebase + ct.pType;

                ok = msvc_rtti_addressslice_contains(tds, ptype_va) &&
                     _msvc_eh_catchabletype_plausible(ctx, &ct, imagebase);

                if(ok) {
                    rd_library_type(ctx, addr, "EH_CatchableType", 0,
                                    RD_TYPE_NONE);

                    char* classtag =
                        rd_strdup(msvc_rtti_classtag_from_typedescriptor(
                            r, ptype_va, is64));

                    if(classtag) {
                        rd_library_name(
                            ctx, addr,
                            rd_format("%s::__catchable_type", classtag));
                    }

                    if(ct.copyFunction) {
                        RDAddress fn_va = imagebase + ct.copyFunction;
                        rd_set_function(ctx, fn_va);

                        if(classtag) {
                            rd_auto_name(
                                ctx, fn_va,
                                rd_format("%s::__copy_ctor", classtag));
                        }
                    }

                    rd_free(classtag);
                }
            }

            addr = ok ? next : addr + sizeof(u32);
        }
    }
}

static void _msvc_eh_find_catchabletypearrays(RDContext* ctx, RDReader* r,
                                              RDAddress imagebase) {
    RDAddressSlice cts = rd_get_all_address_by_type(ctx, "EH_CatchableType");
    if(rd_slice_is_empty(cts)) return;

    RDSegmentSlice segments = rd_get_all_segments(ctx);

    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if(!(seg->perm & RD_SP_R) || (seg->perm & RD_SP_X)) continue;

        RDAddress addr = seg->start_address;

        while(addr < seg->end_address) {
            rd_reader_seek(r, addr);

            EHCatchableTypeArrayHeader hdr;
            bool ok = msvc_eh_read_catchabletypearrayheader(r, &hdr);
            RDAddress entries_va = ok ? (RDAddress)rd_reader_tell(r) : 0;

            if(ok) {
                ok = hdr.nCatchableTypes != 0 &&
                     hdr.nCatchableTypes <= MSVC_EH_MAX_CATCHABLE_TYPES;
            }

            usize arraybytes = (usize)hdr.nCatchableTypes * sizeof(u32);

            if(ok) ok = msvc_rtti_segment_fits(ctx, entries_va, arraybytes);

            for(u32 i = 0; ok && i < hdr.nCatchableTypes; i++) {
                rd_reader_seek(r, entries_va + ((usize)i * sizeof(u32)));
                u32 raw;

                ok = rd_reader_read_le32(r, &raw) &&
                     msvc_rtti_addressslice_contains(cts, imagebase + raw);
            }

            if(ok) {
                rd_library_type(ctx, addr, "EH_CatchableTypeArray", 0,
                                RD_TYPE_NONE);

                for(u32 i = 0; i < hdr.nCatchableTypes; i++) {
                    rd_library_type(ctx, entries_va + ((usize)i * sizeof(u32)),
                                    "u32", 0, RD_TYPE_PTR);
                }

                addr = entries_va + arraybytes;
            }
            else {
                addr += sizeof(u32);
            }
        }
    }
}

static void _msvc_eh_find_throwinfos(RDContext* ctx, RDReader* r,
                                     RDAddress imagebase) {
    RDAddressSlice ctas =
        rd_get_all_address_by_type(ctx, "EH_CatchableTypeArray");
    if(rd_slice_is_empty(ctas)) return;

    RDSegmentSlice segments = rd_get_all_segments(ctx);

    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if(!(seg->perm & RD_SP_R) || (seg->perm & RD_SP_X)) continue;

        RDAddress addr = seg->start_address;

        while(addr < seg->end_address) {
            rd_reader_seek(r, addr);

            EHThrowInfo ti;
            bool ok = msvc_eh_read_throwinfo(r, &ti);
            RDAddress next = ok ? (RDAddress)rd_reader_tell(r) : 0;

            if(ok) {
                ok = !(ti.attributes & ~(u32)MSVC_EH_ATTR_MASK) &&
                     msvc_rtti_addressslice_contains(
                         ctas, imagebase + ti.pCatchableTypeArray) &&
                     (!ti.pmfnUnwind || msvc_rtti_segment_exec_ok(
                                            ctx, imagebase + ti.pmfnUnwind)) &&
                     (!ti.pForwardCompat ||
                      msvc_rtti_segment_exec_ok(ctx,
                                                imagebase + ti.pForwardCompat));
            }

            if(ok) {
                rd_library_type(ctx, addr, "EH_ThrowInfo", 0, RD_TYPE_NONE);
                rd_library_name(ctx, addr,
                                rd_format("__throwinfo_%" PRIX64, addr));

                if(ti.pmfnUnwind) {
                    RDAddress fn_va = imagebase + ti.pmfnUnwind;
                    rd_set_function(ctx, fn_va);
                    rd_auto_name(
                        ctx, fn_va,
                        rd_format("sub_%" PRIX64 "__unwind_dtor", fn_va));
                }
            }

            addr = ok ? next : addr + sizeof(u32);
        }
    }
}

static bool _msvc_eh_funcinfo_magic_ok(u32 magic_and_flags) {
    u32 magic = magic_and_flags & MSVC_EH_FUNCINFO_MAGIC_MASK;
    return (magic ^ MSVC_EH_FUNCINFO_MAGIC_BASE) <= MSVC_EH_FUNCINFO_MAGIC_SPAN;
}

static RDAddress _msvc_eh_process_funcinfo(RDContext* ctx, RDReader* r,
                                           RDAddress va, RDAddress imagebase) {
    rd_reader_seek(r, va);

    EHFuncInfo fi;
    if(!msvc_eh_read_funcinfo(r, &fi)) return 0;

    RDAddress after_header = (RDAddress)rd_reader_tell(r);

    if(!_msvc_eh_funcinfo_magic_ok(fi.magicAndBbtFlags)) return 0;
    if(fi.maxState > MSVC_EH_MAX_UNWIND_STATES) return 0;
    if(fi.nTryBlocks > MSVC_EH_MAX_TRY_BLOCKS) return 0;

    RDAddress unwindmap_va = imagebase + fi.pUnwindMap;
    RDAddress tryblockmap_va = imagebase + fi.pTryBlockMap;

    if(fi.pUnwindMap && !msvc_rtti_segment_ok(ctx, unwindmap_va)) return 0;
    if(fi.pTryBlockMap && !msvc_rtti_segment_ok(ctx, tryblockmap_va)) return 0;

    // Validated: tag it and start seeding functions.
    rd_library_type(ctx, va, "EH_FuncInfo", 0, RD_TYPE_NONE);
    rd_library_name(ctx, va, rd_format("__funcinfo_%" PRIX64, va));

    RDAddressSlice tds = rd_get_all_address_by_type(ctx, "RTTI_TypeDescriptor");
    bool is64 = imagebase != 0;

    // Unwind map: 'action' entries are cleanup funclets run while
    // unwinding through that state (destructors for locals already fully
    // constructed at that point).
    // Reached only by the frame handler.
    if(fi.pUnwindMap) {
        rd_reader_seek(r, unwindmap_va);

        for(u32 i = 0; i < fi.maxState; i++) {
            RDAddress entry_va = (RDAddress)rd_reader_tell(r);
            EHUnwindMapEntry ue;
            if(!msvc_eh_read_unwindmapentry(r, &ue)) break;

            rd_library_type(ctx, entry_va, "EH_UnwindMapEntry", 0,
                            RD_TYPE_NONE);

            if(ue.action) {
                RDAddress fn_va = imagebase + ue.action;

                if(msvc_rtti_segment_exec_ok(ctx, fn_va)) {
                    rd_set_function(ctx, fn_va);
                    rd_auto_name(ctx, fn_va,
                                 rd_format("__unwind_funclet_%" PRIX64, fn_va));
                }
            }
        }
    }

    // Try block map: each entry's handler array holds the catch blocks.
    // addressOfHandler is the actual payoff here.
    // A catch block entry point invoked only by the frame handler, never by any
    // call/jmp.
    if(fi.pTryBlockMap) {
        rd_reader_seek(r, tryblockmap_va);

        for(u32 i = 0; i < fi.nTryBlocks; i++) {
            RDAddress tb_va = (RDAddress)rd_reader_tell(r);
            EHTryBlockMapEntry tb;
            if(!msvc_eh_read_tryblockmapentry(r, &tb)) break;

            RDAddress next_tb = (RDAddress)rd_reader_tell(r);

            if(tb.nCatches <= MSVC_EH_MAX_HANDLERS_PER_TRY) {
                rd_library_type(ctx, tb_va, "EH_TryBlockMapEntry", 0,
                                RD_TYPE_NONE);

                RDAddress handlers_va = imagebase + tb.pHandlerArray;

                if(tb.pHandlerArray && msvc_rtti_segment_ok(ctx, handlers_va)) {
                    rd_reader_seek(r, handlers_va);

                    for(u32 j = 0; j < tb.nCatches; j++) {
                        RDAddress h_va = (RDAddress)rd_reader_tell(r);
                        EHHandlerType h;
                        if(!msvc_eh_read_handlertype(r, &h)) break;

                        rd_library_type(ctx, h_va, "EH_HandlerType", 0,
                                        RD_TYPE_NONE);

                        const char* classtag = NULL;

                        if(h.pType) {
                            // typed catch: cross-check against RTTI, same
                            // as CatchableType.pType .
                            // pType == 0 is catch(...), always valid, nothing
                            // to check.
                            RDAddress ptype_va = imagebase + h.pType;

                            if(msvc_rtti_addressslice_contains(tds, ptype_va)) {
                                classtag =
                                    msvc_rtti_classtag_from_typedescriptor(
                                        r, ptype_va, is64);
                            }
                        }

                        if(h.addressOfHandler) {
                            RDAddress fn_va = imagebase + h.addressOfHandler;

                            if(msvc_rtti_segment_exec_ok(ctx, fn_va)) {
                                rd_set_function(ctx, fn_va);

                                if(classtag) {
                                    rd_auto_name(
                                        ctx, fn_va,
                                        rd_format("%s::__catch_handler",
                                                  classtag));
                                }
                                else {
                                    rd_auto_name(
                                        ctx, fn_va,
                                        rd_format("__catch_handler_%" PRIX64,
                                                  fn_va));
                                }
                            }
                        }
                    }
                }
            }

            rd_reader_seek(r, next_tb);
        }
    }

    return after_header;
}

static void _msvc_eh_find_funcinfo_scan(RDContext* ctx, RDReader* r) {
    RDSegmentSlice segments = rd_get_all_segments(ctx);

    const RDSegment** it;
    rd_slice_each(it, segments) {
        const RDSegment* seg = *it;
        if(!(seg->perm & RD_SP_R) || (seg->perm & RD_SP_X)) continue;

        RDAddress addr = seg->start_address;

        while(addr < seg->end_address) {
            RDAddress next = _msvc_eh_process_funcinfo(ctx, r, addr, 0);
            addr = next ? next : addr + sizeof(u32);
        }
    }
}

static void _msvc_eh_find_funcinfo_anchored(RDContext* ctx, RDReader* r,
                                            RDAddress imagebase) {
    RDAddressSlice candidates =
        rd_get_all_address_by_type(ctx, "PE_EH_FUNCINFO_CANDIDATE");
    if(rd_slice_is_empty(candidates)) return;

    const RDAddress* it;
    rd_slice_each(it, candidates) {
        _msvc_eh_process_funcinfo(ctx, r, *it, imagebase);
    }
}

static RDAddress _msvc_eh_get_imagebase_v1(RDContext* ctx, RDReader* r) {
    RDAddressSlice locators =
        rd_get_all_address_by_type(ctx, "RTTI_CompleteObjectLocator64");
    if(rd_slice_is_empty(locators)) return 0;

    RDAddress locator_addr = 0;
    const RDAddress* it;
    rd_slice_each(it, locators) {
        locator_addr = *it;
        break;
    }

    if(!locator_addr) return 0;

    rd_reader_seek(r, locator_addr);
    RTTICompleteObjectLocator objlocator;

    if(!msvc_rtti_read_completeobjectlocator(r, &objlocator)) return 0;
    if(objlocator.signature != MSVC_RTTI_SIGNATURE_V1 || !objlocator.pSelf)
        return 0;

    return locator_addr - objlocator.pSelf;
}

static void msvc_eh_execute(RDContext* ctx) {
    rd_kb_load(ctx, "compiler/msvc/eh");

    RDReader* r = rd_get_reader(ctx);

    // V0 (x86): on-disk pointers are absolute VAs -> imagebase 0
    _msvc_eh_find_catchabletypes(ctx, r, 0);
    _msvc_eh_find_catchabletypearrays(ctx, r, 0);
    _msvc_eh_find_throwinfos(ctx, r, 0);

    // V1 (x64/ARM64): on-disk pointers are image-relative offsets
    RDAddress imagebase = _msvc_eh_get_imagebase_v1(ctx, r);

    // FuncInfo's blind scan interprets its fields
    // as absolute (imagebase 0).
    // Running it against a module that's actually V1/x64 would
    // silently misinterpret FuncInfo's relative fields as absolute ones.
    if(!imagebase) _msvc_eh_find_funcinfo_scan(ctx, r);

    if(imagebase) {
        _msvc_eh_find_catchabletypes(ctx, r, imagebase);
        _msvc_eh_find_catchabletypearrays(ctx, r, imagebase);
        _msvc_eh_find_throwinfos(ctx, r, imagebase);
        _msvc_eh_find_funcinfo_anchored(ctx, r, imagebase);
    }
}

const RDAnalyzerPlugin MSVC_EH = {
    .level = RD_API_LEVEL,
    .id = "compiler_msvc_eh",
    .name = "Decode MSVC Exception Handling Tables",
    .flags = RD_AF_RUNONCE | RD_AF_EXPERIMENTAL,
    .order = 1001, // after RTTI_MSVC: depends on its TypeDescriptor index
    .execute = msvc_eh_execute,
};
