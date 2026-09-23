#include "load.h"
#include "format/dbi.h"
#include "format/pdb.h"
#include "format/symbols.h"
#include "request.h"
#include <ctype.h>
#include <string.h>

#define PDB_SERVER_CONNECT_TIMEOUT_MS 5000

static bool _pdb_validate_path_component(const char* s) {
    if(!s || !*s) return false;
    if(strstr(s, "..")) return false; // no traversal

    for(const char* p = s; *p; p++) {
        u8 c = (u8)*p;
        if(c < 0x20 || c == 0x7F) return false; // no control chars
    }

    return true;
}

static bool _pdb_try_case_variants(const char* dir, const char* name,
                                   RDScratchBuffer* out) {
    const char* exact = rd_path_join(dir, name);
    if(rd_path_isfile(exact)) {
        rd_scratch_puts(out, exact);
        return true;
    }

    RDScratchBuffer* lower = rd_scratch_create();
    for(const char* p = name; *p; p++)
        rd_scratch_putchar(lower, (char)tolower((u8)*p));

    const char* candidate = rd_path_join(dir, rd_scratch_data(lower));
    bool found = rd_path_isfile(candidate);
    if(found) rd_scratch_puts(out, candidate);

    rd_scratch_destroy(lower);
    return found;
}

// Tries `binary_dir/pdb_name` (and its lowercase variant). On success,
// leaves the verified file open in *out_pdb and the resolved path in
// out_path - caller owns both.
static bool _pdb_resolve_local(const char* binary_dir, const char* pdb_name,
                               const char* expected_guid, u32 expected_age,
                               RDScratchBuffer* out_path, PDBFile* out_pdb) {
    RDScratchBuffer* candidate_buf = rd_scratch_create();
    bool has_candidate =
        _pdb_try_case_variants(binary_dir, pdb_name, candidate_buf);

    if(!has_candidate) {
        rd_scratch_destroy(candidate_buf);
        return false;
    }

    const char* candidate = rd_scratch_data(candidate_buf);
    bool ok = pdb_open(candidate, out_pdb) &&
              pdb_verify(out_pdb, expected_guid, expected_age);

    if(ok)
        rd_scratch_puts(out_path, candidate);
    else
        pdb_close(out_pdb);

    rd_scratch_destroy(candidate_buf);
    return ok;
}

// Fetches from the symbol server. On success, leaves the verified
// content open in *out_pdb (backed by `reply`, which the caller must
// keep alive at least as long as out_pdb - see pdb_open_data).
static bool _pdb_resolve_network(const char* pdb_name,
                                 const char* expected_guid, u32 expected_age,
                                 RDScratchBuffer* reply, PDBFile* out_pdb) {
    if(!pdb_fetch_from_server(pdb_name, expected_guid, expected_age, reply,
                              PDB_SERVER_CONNECT_TIMEOUT_MS))
        return false;

    bool ok = pdb_open_data(rd_scratch_data(reply), rd_scratch_length(reply),
                            out_pdb) &&
              pdb_verify(out_pdb, expected_guid, expected_age);

    if(!ok) pdb_close(out_pdb);
    return ok;
}

static void _pdb_load(RDContext* ctx, const char* pdb_input,
                      const char* pdb_guid, u32 pdb_age) {
    PDBFile pdb = {0};
    PDBStream dbi_stream = {0};
    PDBSectionHeaderList sections = {0};
    RDScratchBuffer* resolved_buf = NULL;
    RDScratchBuffer* server_reply = NULL;
    const char* found_path = NULL; // set once, at the point of success
    bool pdb_ready = false;

    if(!_pdb_validate_path_component(pdb_input)) goto cleanup;

    // 1. try the given path exactly as-is
    if(rd_path_isfile(pdb_input)) {
        pdb_ready =
            pdb_open(pdb_input, &pdb) && pdb_verify(&pdb, pdb_guid, pdb_age);
        if(pdb_ready)
            found_path = pdb_input;
        else
            pdb_close(&pdb);
    }

    // 2. same directory as the analyzed binary, with case fallback
    if(!pdb_ready) {
        resolved_buf = rd_scratch_create();
        const char* basename = rd_path_filename(pdb_input);
        const char* binary_dir = rd_get_working_dir(ctx);

        if(_pdb_resolve_local(binary_dir, basename, pdb_guid, pdb_age,
                              resolved_buf, &pdb)) {
            pdb_ready = true;
            found_path = rd_scratch_data(resolved_buf);
        }
    }

    // 3. symbol server, if network is enabled
    if(!pdb_ready) {
        server_reply = rd_scratch_create();
        const char* basename = rd_path_filename(pdb_input);

        if(_pdb_resolve_network(basename, pdb_guid, pdb_age, server_reply,
                                &pdb)) {
            pdb_ready = true;
            found_path = "<symbol server>";
        }
    }

    if(!pdb_ready) {
        RD_LOG_FAIL("PDB not found or verification failed: %s", pdb_input);
        goto cleanup;
    }

    RD_LOG_INFO("loading PDB '%s'", found_path);

    if(!pdb_read_stream_by_index(&pdb, PDB_STREAM_DBI, &dbi_stream))
        goto cleanup;

    PDBDbiHeader dbi;
    if(!pdb_read_dbi_header(&dbi_stream, &dbi)) goto cleanup;

    PDBDbiDbgHeader dbg;
    if(!pdb_read_dbi_dbg_header(&dbi_stream, &dbi, &dbg)) goto cleanup;
    pdb_stream_destroy(&dbi_stream);

    if(!pdb_read_section_headers(&pdb, dbg.SectionHdr, &sections)) goto cleanup;

    pdb_apply_symbols(ctx, &pdb, dbi.SymRec, &sections);

cleanup:
    rd_scratch_destroy(server_reply);
    rd_scratch_destroy(resolved_buf);
    pdb_section_header_list_destroy(&sections);
    pdb_stream_destroy(&dbi_stream);
    pdb_close(&pdb);
}

static void pdb_debuginfo_execute(RDContext* ctx) {
    RDAddressSlice types = rd_get_all_address_by_type(ctx, "CV_INFO_PDB70");

    if(rd_slice_length(types) == 0) {
        RD_LOG_INFO("no PDB 7.0 debug info found");
        return;
    }

    if(rd_slice_length(types) > 1) {
        RD_LOG_WARN("found %zu CV_INFO_PDB70 records (expected 1), skipping",
                    rd_slice_length(types));
        return;
    }

    PDBCvInfo70 pdb_hdr;
    RDAddress address = rd_slice_at(types, 0);

    RDReader* r = rd_get_reader(ctx);
    rd_reader_seek(r, (u64)address);

    if(!pdb_read_cv_info_70(r, &pdb_hdr)) return;

    usize n;
    const char* pdb_filepath = rd_reader_peek_str(r, &n);
    if(!pdb_filepath) return;

    const char* pdb_guid = rd_format(
        "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X", pdb_hdr.GuidData1,
        pdb_hdr.GuidData2, pdb_hdr.GuidData3, pdb_hdr.GuidData4[0],
        pdb_hdr.GuidData4[1], pdb_hdr.GuidData4[2], pdb_hdr.GuidData4[3],
        pdb_hdr.GuidData4[4], pdb_hdr.GuidData4[5], pdb_hdr.GuidData4[6],
        pdb_hdr.GuidData4[7]);

    _pdb_load(ctx, pdb_filepath, pdb_guid, pdb_hdr.Age);
}

const RDAnalyzerPlugin PDB_DEBUGINFO = {
    .id = "pdb_debuginfo",
    .name = "Load PDB Debug Symbols",
    .flags = RD_AF_RUNONCE,
    .execute = pdb_debuginfo_execute,
};
