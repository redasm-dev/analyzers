#include "symbols.h"

#define S_PUB32 0x110E

// PublicSymFlags
#define CVPSF_FUNCTION 0x02

typedef struct PDBsPub32 {
    u32 Flags;
    u32 Offset;
    u16 Section;
} PDBsPub32;

static bool _pdb_read_pub32(RDReader* r, PDBsPub32* out) {
    rd_reader_read_le32(r, &out->Flags);
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
            PDBsPub32 pub;
            if(_pdb_read_pub32(r, &pub)) {
                usize namelen;
                const char* name = rd_reader_read_str(r, &namelen);

                if(name && namelen > 0) {
                    RDAddress rva;
                    if(pdb_section_va(ctx, sections, pub.Section, pub.Offset,
                                      &rva)) {
                        if(pub.Flags & CVPSF_FUNCTION)
                            rd_set_function(ctx, rva);
                        rd_library_name(ctx, rva, name);
                        applied++;
                    }
                }
            }
        }

        // always advance to next record regardless of kind
        rd_reader_seek(r, rec_end);
    }

    RD_LOG_INFO("applied %u symbols", applied);
    rd_reader_close(r);
    pdb_stream_destroy(&s);
    return applied > 0;
}
