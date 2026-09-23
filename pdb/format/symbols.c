#include "symbols.h"

#define S_LDATA32 0x110C
#define S_GDATA32 0x110D
#define S_PUB32 0x110E

// PublicSymFlags (S_PUB32 only)
#define CVPSF_FUNCTION 0x02

typedef struct PDBSPub32 {
    u32 Flags;
    u32 Offset;
    u16 Section;
} PDBSPub32;

typedef struct PDBSData32 {
    u32 TypeIndex;
    u32 Offset;
    u16 Section;
} PDBSData32;

static bool _pdb_read_pub32(RDReader* r, PDBSPub32* out) {
    rd_reader_read_le32(r, &out->Flags);
    rd_reader_read_le32(r, &out->Offset);
    rd_reader_read_le16(r, &out->Section);
    return !rd_reader_has_error(r);
}

static bool _pdb_read_data32(RDReader* r, PDBSData32* out) {
    rd_reader_read_le32(r, &out->TypeIndex);
    rd_reader_read_le32(r, &out->Offset);
    rd_reader_read_le16(r, &out->Section);
    return !rd_reader_has_error(r);
}

bool pdb_apply_symbols(RDContext* ctx, PDBFile* pdb, u16 sym_stream_idx,
                       const PDBSectionHeaderList* sections) {
    PDBStream s = {0};
    if(!pdb_read_stream_by_index(pdb, sym_stream_idx, &s)) return false;

    RDReader* r = rd_reader_open_data(s.data, s.size);
    u32 applied = 0;

    while(!rd_reader_at_end(r)) {
        u16 len, kind;
        if(!rd_reader_read_le16(r, &len)) break;
        if(!rd_reader_read_le16(r, &kind)) break;
        if(len < 2) break;

        u64 rec_end = rd_reader_tell(r) + (len - 2);

        if(kind == S_PUB32) {
            PDBSPub32 pub;
            if(_pdb_read_pub32(r, &pub)) {
                usize namelen;
                const char* name = rd_reader_read_str(r, &namelen);

                if(name && namelen > 0) {
                    RDAddress va;
                    if(pdb_section_va(ctx, sections, pub.Section, pub.Offset,
                                      &va)) {
                        if(pub.Flags & CVPSF_FUNCTION) rd_set_function(ctx, va);
                        rd_library_name(ctx, va, name);
                        applied++;
                    }
                }
            }
        }
        else if(kind == S_LDATA32 || kind == S_GDATA32) {
            // static/global data symbols: always data, never a function.
            // No flags field to check.
            PDBSData32 data;
            if(_pdb_read_data32(r, &data)) {
                usize namelen;
                const char* name = rd_reader_read_str(r, &namelen);

                if(name && namelen > 0) {
                    RDAddress va;
                    if(pdb_section_va(ctx, sections, data.Section, data.Offset,
                                      &va)) {
                        rd_library_name(ctx, va, name);
                        applied++;
                    }
                }
            }
        }

        // always advance to the next record regardless of kind.
        // An unrecognized or malformed record must never stall the walk
        rd_reader_seek(r, rec_end);
    }

    RD_LOG_INFO("applied %u symbols", applied);
    rd_reader_close(r);
    pdb_stream_destroy(&s);
    return applied > 0;
}
