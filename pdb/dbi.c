#include "dbi.h"

#define PDB_DBI_HEADER_SIZE 64

bool pdb_read_dbi_header(const PDBStream* s, PDBDbiHeader* out) {
    RDReader* r = rd_reader_open_data(s->data, s->size);
    if(!r) return false;

    rd_reader_read_le32(r, (u32*)&out->VerSig);
    rd_reader_read_le32(r, &out->VerHdr);
    rd_reader_read_le32(r, &out->Age);
    rd_reader_read_le16(r, &out->GSI);
    rd_reader_read_le16(r, &out->Build);
    rd_reader_read_le16(r, &out->PSI);
    rd_reader_read_le16(r, &out->PdbDllVer);
    rd_reader_read_le16(r, &out->SymRec);
    rd_reader_read_le16(r, &out->PdbDllRbld);
    rd_reader_read_le32(r, (u32*)&out->ModInfoSize);
    rd_reader_read_le32(r, (u32*)&out->SecContrSize);
    rd_reader_read_le32(r, (u32*)&out->SecMapSize);
    rd_reader_read_le32(r, (u32*)&out->SrcInfoSize);
    rd_reader_read_le32(r, (u32*)&out->TypeSrvSize);
    rd_reader_read_le32(r, &out->MFCIdx);
    rd_reader_read_le32(r, (u32*)&out->DbgHdrSize);
    rd_reader_read_le32(r, (u32*)&out->ECSize);
    rd_reader_read_le16(r, &out->Flags);
    rd_reader_read_le16(r, &out->Machine);

    return rd_reader_close(r);
}

bool pdb_read_dbi_dbg_header(const PDBStream* s, const PDBDbiHeader* dbi,
                             PDBDbiDbgHeader* out) {
    u32 dbg_off = PDB_DBI_HEADER_SIZE + (u32)dbi->ModInfoSize +
                  (u32)dbi->SecContrSize + (u32)dbi->SecMapSize +
                  (u32)dbi->SrcInfoSize + (u32)dbi->TypeSrvSize +
                  (u32)dbi->ECSize;

    RDReader* r = rd_reader_open_data(s->data, s->size);
    if(!r) return false;

    rd_reader_seek(r, dbg_off);
    rd_reader_read_le16(r, &out->FPO);
    rd_reader_read_le16(r, &out->Exception);
    rd_reader_read_le16(r, &out->Fixup);
    rd_reader_read_le16(r, &out->OmapToSrc);
    rd_reader_read_le16(r, &out->OmapFromSrc);
    rd_reader_read_le16(r, &out->SectionHdr);
    rd_reader_read_le16(r, &out->TokenRidMap);
    rd_reader_read_le16(r, &out->Xdata);
    rd_reader_read_le16(r, &out->Pdata);
    rd_reader_read_le16(r, &out->NewFPO);
    rd_reader_read_le16(r, &out->SectionHdrOrig);

    return rd_reader_close(r);
}

bool pdb_read_section_headers(PDBFile* pdb, u16 stream_idx,
                              PDBSectionHeaderList* out) {
    if(stream_idx == 0xFFFF) {
        RD_LOG_FAIL("no section header stream");
        return false;
    }

    PDBStream s = {0};
    if(!pdb_read_stream_by_index(pdb, stream_idx, &s)) return false;

    if(s.size % PDB_SECTION_HEADER_SIZE != 0) {
        RD_LOG_FAIL("section header stream size not a multiple of %u",
                    PDB_SECTION_HEADER_SIZE);
        pdb_stream_destroy(&s);
        return false;
    }

    out->count = s.size / PDB_SECTION_HEADER_SIZE;
    out->headers = rd_alloc0(out->count, sizeof(PDBSectionHeader));

    RDReader* r = rd_reader_open_data(s.data, s.size);

    for(u32 i = 0; i < out->count; i++) {
        rd_reader_read_exact(r, out->headers[i].Name, 8);
        rd_reader_read_le32(r, &out->headers[i].VirtualSize);
        rd_reader_read_le32(r, &out->headers[i].VirtualAddress);
        rd_reader_read_le32(r, &out->headers[i].SizeOfRawData);
        rd_reader_read_le32(r, &out->headers[i].PointerToRawData);
        rd_reader_skip(r, 16); // remaining fields unused
    }

    bool ok = rd_reader_close(r);
    pdb_stream_destroy(&s);

    if(!ok) pdb_section_header_list_destroy(out);
    return ok;
}

void pdb_section_header_list_destroy(PDBSectionHeaderList* list) {
    rd_free(list->headers);
    *list = (PDBSectionHeaderList){0};
}

bool pdb_section_va(const RDContext* ctx, const PDBSectionHeaderList* list,
                    u16 section, u32 offset, RDAddress* out) {
    if(section == 0 || section > list->count) return false;

    *out = rd_get_base_address(ctx) +
           list->headers[section - 1].VirtualAddress + offset;
    return true;
}
