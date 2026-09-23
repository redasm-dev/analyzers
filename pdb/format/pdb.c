#include "pdb.h"
#include <string.h>

#define PDB_VERSION_2 "Microsoft C/C++ program database 2.00"

#define PDB_VERSION_7                                                          \
    "Microsoft C/C++ MSF 7.00\r\n\x1a"                                         \
    "DS\x00\x00\x00"

#define PDB_INFO_HEADER_SIZE 28

static bool _pdb_validate_pages(const PDBSuperBlock* sb, const u32* pages,
                                u32 n) {
    for(u32 i = 0; i < n; i++) {
        if(pages[i] >= sb->NumPages) {
            RD_LOG_FAIL("page %u out of range", pages[i]);
            return false;
        }
    }
    return true;
}

static u32 _pdb_n_pages(const PDBFile* pdb, u32 size) {
    return (size + pdb->super_block.PageSize - 1) / pdb->super_block.PageSize;
}

static bool _pdb_read_directory(PDBFile* pdb) {
    u32 n_dir_pages = _pdb_n_pages(pdb, pdb->super_block.RootSize);
    u32 blockmap_off =
        pdb->super_block.RootPageIndexes * pdb->super_block.PageSize;

    u32* dir_pages = rd_alloc0(n_dir_pages, sizeof(u32));
    rd_reader_seek(pdb->r, blockmap_off);

    bool ok =
        rd_reader_read_exact(pdb->r, dir_pages, n_dir_pages * sizeof(u32));
    if(!ok) {
        rd_free(dir_pages);
        return false;
    }

    if(!_pdb_validate_pages(&pdb->super_block, dir_pages, n_dir_pages)) {
        rd_free(dir_pages);
        return false;
    }

    // assemble directory stream
    u8* dir_data = rd_alloc0(1, pdb->super_block.RootSize);

    for(u32 i = 0; i < n_dir_pages; i++) {
        u32 off = i * pdb->super_block.PageSize;
        u32 sz = (off + pdb->super_block.PageSize > pdb->super_block.RootSize)
                     ? pdb->super_block.RootSize - off
                     : pdb->super_block.PageSize;

        rd_reader_seek(pdb->r, dir_pages[i] * (uptr)pdb->super_block.PageSize);

        if(!rd_reader_read_exact(pdb->r, dir_data + off, sz)) {
            rd_free(dir_pages);
            rd_free(dir_data);
            return false;
        }
    }

    rd_free(dir_pages);

    // parse stream count
    memcpy(&pdb->n_streams, dir_data, sizeof(u32));

    if(pdb->n_streams == 0 || pdb->n_streams > 1024) {
        RD_LOG_FAIL("bad stream count %u", pdb->n_streams);
        rd_free(dir_data);
        return false;
    }

    // parse sizes
    pdb->sizes = rd_alloc0(pdb->n_streams, sizeof(u32));
    pdb->stream_pages = (u32**)rd_alloc0(pdb->n_streams, sizeof(u32*));
    pdb->stream_n_pages = rd_alloc0(pdb->n_streams, sizeof(u32));

    memcpy(pdb->sizes, dir_data + sizeof(u32), pdb->n_streams * sizeof(u32));

    // parse per-stream page lists
    u32 dir_off = sizeof(u32) + (pdb->n_streams * sizeof(u32));

    for(u32 i = 0; i < pdb->n_streams; i++) {
        if(pdb->sizes[i] == 0 || pdb->sizes[i] == 0xFFFFFFFF) continue;

        u32 np = _pdb_n_pages(pdb, pdb->sizes[i]);
        pdb->stream_n_pages[i] = np;
        pdb->stream_pages[i] = rd_alloc0(np, sizeof(u32));
        memcpy(pdb->stream_pages[i], dir_data + dir_off, np * sizeof(u32));
        dir_off += np * sizeof(u32);
    }

    rd_free(dir_data);
    return true;
}

static bool _pdb_open_file(PDBFile* out) {
    if(!pdb_read_superblock(out->r, &out->super_block)) return false;
    if(!pdb_check_rootsize(&out->super_block)) return false;
    if(!pdb_check_rootpageindex(out->r, &out->super_block)) return false;
    if(!pdb_check_pagesize(&out->super_block)) return false;
    if(!pdb_check_numblocks(out->r, &out->super_block)) return false;

    if(!_pdb_read_directory(out)) {
        pdb_close(out);
        return false;
    }

    PDBStream s = {0};
    if(!pdb_read_stream_by_index(out, PDB_STREAM_INFO, &s)) return false;

    bool ok = pdb_read_info_header(&s, &out->info);
    pdb_stream_destroy(&s);

    if(!ok) {
        RD_LOG_FAIL("cannot read PDBInfoHeader");
        return false;
    }

    out->guid = rd_strdup(
        rd_format("%02X%02X%02X%02X%02X%02X%02X%02X"
                  "%02X%02X%02X%02X%02X%02X%02X%02X",
                  out->info.Guid[0], out->info.Guid[1], out->info.Guid[2],
                  out->info.Guid[3], out->info.Guid[4], out->info.Guid[5],
                  out->info.Guid[6], out->info.Guid[7], out->info.Guid[8],
                  out->info.Guid[9], out->info.Guid[10], out->info.Guid[11],
                  out->info.Guid[12], out->info.Guid[13], out->info.Guid[14],
                  out->info.Guid[15]));

    return true;
}

bool pdb_read_superblock(RDReader* r, PDBSuperBlock* out) {
    rd_reader_seek(r, 0);

    if(!rd_reader_read_exact(r, out->FileMagic, PDB_SUPERBLOCK_MAGIC_LENGTH))
        return false;

    if(memcmp(out->FileMagic, PDB_VERSION_7, PDB_SUPERBLOCK_MAGIC_LENGTH) !=
       0) {
        if(memcmp(out->FileMagic, PDB_VERSION_2, 37) == 0)
            RD_LOG_FAIL("MSF 2.0 (NB10) not supported");
        else
            RD_LOG_FAIL("not a PDB file");
        return false;
    }

    rd_reader_read_le32(r, &out->PageSize);
    rd_reader_read_le32(r, &out->FlagPage);
    rd_reader_read_le32(r, &out->NumPages);
    rd_reader_read_le32(r, &out->RootSize);
    rd_reader_read_le32(r, &out->Reserved);
    rd_reader_read_le32(r, &out->RootPageIndexes);
    return !rd_reader_has_error(r);
}

bool pdb_check_pagesize(const PDBSuperBlock* pdb) {
    switch(pdb->PageSize) {
        case 512:
        case 1024:
        case 2048:
        case 4096: return true;

        default: break;
    }

    RD_LOG_FAIL("bad page size %d", pdb->PageSize);
    return false;
}

bool pdb_check_numblocks(const RDReader* r, const PDBSuperBlock* pdb) {
    if((u64)pdb->NumPages * pdb->PageSize != rd_reader_get_length(r)) {
        RD_LOG_FAIL("bad block size %d (truncated?)", pdb->PageSize);
        return false;
    }

    return true;
}

bool pdb_check_rootsize(const PDBSuperBlock* pdb) {
    if(!pdb->RootSize) {
        RD_LOG_FAIL("empty root stream");
        return false;
    }

    return true;
}

bool pdb_check_rootpageindex(const RDReader* r, const PDBSuperBlock* pdb) {
    if(pdb->RootPageIndexes * pdb->PageSize >= (u32)rd_reader_get_length(r)) {
        RD_LOG_FAIL("stream directory out of range (truncated?)");
        return false;
    }

    return true;
}

bool pdb_read_stream(RDReader* r, const PDBSuperBlock* sb, const u32* pages,
                     u32 n_pages, PDBStream* out) {
    out->size = n_pages * sb->PageSize;
    out->data = rd_alloc0(1, out->size);

    for(u32 i = 0; i < n_pages; i++) {
        rd_reader_seek(r, pages[i] * (uptr)sb->PageSize);
        if(!rd_reader_read_exact(r, out->data + (i * (uptr)sb->PageSize),
                                 sb->PageSize)) {
            pdb_stream_destroy(out);
            return false;
        }
    }
    return true;
}

bool pdb_read_info_header(const PDBStream* s, PDBInfoHeader* out) {
    RDReader* r = rd_reader_open_data(s->data, s->size);
    if(!r) return false;

    rd_reader_read_le32(r, &out->Version);
    rd_reader_read_le32(r, &out->Signature);
    rd_reader_read_le32(r, &out->Age);
    rd_reader_read_exact(r, out->Guid, 16);

    return rd_reader_close(r);
}

void pdb_stream_destroy(PDBStream* s) {
    rd_free(s->data);
    *s = (PDBStream){0};
}

bool pdb_open(const char* path, PDBFile* out) {
    out->r = rd_reader_open(path);
    return out->r ? _pdb_open_file(out) : false;
}

bool pdb_open_data(const void* data, usize size, PDBFile* out) {
    out->r = rd_reader_open_data(data, size);
    return out->r ? _pdb_open_file(out) : false;
}

bool pdb_verify(PDBFile* pdb, const char* expected_guid, u32 expected_age) {
    if(strcmp(pdb->guid, expected_guid) != 0) {
        RD_LOG_FAIL("PDB GUID mismatch");
        return false;
    }

    if(pdb->info.Age != expected_age) {
        RD_LOG_FAIL("PDB age mismatch: expected %u got %u", expected_age,
                    pdb->info.Age);
        return false;
    }

    RD_LOG_INFO("PDB is valid");
    return true;
}

bool pdb_read_stream_by_index(PDBFile* pdb, u32 idx, PDBStream* out) {
    if(idx >= pdb->n_streams) {
        RD_LOG_FAIL("stream index %u out of range", idx);
        return false;
    }

    if(pdb->sizes[idx] == 0 || pdb->sizes[idx] == 0xFFFFFFFF) {
        RD_LOG_FAIL("stream %u is empty or deleted", idx);
        return false;
    }

    if(!_pdb_validate_pages(&pdb->super_block, pdb->stream_pages[idx],
                            pdb->stream_n_pages[idx]))
        return false;

    bool ok = pdb_read_stream(pdb->r, &pdb->super_block, pdb->stream_pages[idx],
                              pdb->stream_n_pages[idx], out);
    if(ok) out->size = pdb->sizes[idx]; // trim to actual size
    return ok;
}

void pdb_close(PDBFile* pdb) {
    if(!pdb) return;

    for(u32 i = 0; i < pdb->n_streams; i++)
        rd_free(pdb->stream_pages[i]);

    rd_free((void*)pdb->stream_pages);
    rd_free(pdb->stream_n_pages);
    rd_free(pdb->sizes);
    rd_reader_close(pdb->r);
    rd_free(pdb->guid);

    *pdb = (PDBFile){0};
}

bool pdb_read_cv_info_70(RDReader* r, PDBCvInfo70* pdb) {
    rd_reader_read_le32(r, &pdb->CvSignature);
    rd_reader_read_le32(r, &pdb->GuidData1);
    rd_reader_read_le16(r, &pdb->GuidData2);
    rd_reader_read_le16(r, &pdb->GuidData3);
    rd_reader_read_exact(r, pdb->GuidData4, sizeof(pdb->GuidData4));
    rd_reader_read_le32(r, &pdb->Age);

    return !rd_reader_has_error(r);
}
