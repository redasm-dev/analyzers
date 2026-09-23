#include "request.h"

#define PDB_SYMBOL_SERVER_DEFAULT "https://msdl.microsoft.com/download/symbols"

static bool _pdb_fetch(const char* server, const char* pdb_name,
                       const char* guid, u32 age, RDScratchBuffer* url_buf,
                       RDScratchBuffer* reply, u32 timeout_ms) {
    rd_scratch_puts(url_buf, server);
    rd_scratch_putchar(url_buf, '/');
    rd_scratch_puts(url_buf, pdb_name);
    rd_scratch_putchar(url_buf, '/');
    rd_scratch_puts(url_buf, guid);
    rd_scratch_putf(url_buf, "%X", age);
    rd_scratch_putchar(url_buf, '/');
    rd_scratch_puts(url_buf, pdb_name);

    const char* full_url = rd_scratch_data(url_buf);
    RD_LOG_INFO("fetching %s", full_url);

    RDNetStatus st = rd_net_get(full_url, reply, timeout_ms);
    if(st.ok && st.code == 200) return true;

    RD_LOG_WARN("fetch failed (%s)", st.ok ? "HTTP error" : "transport error");
    return false;
}

bool pdb_fetch_from_server(const char* pdb_name, const char* guid, u32 age,
                           RDScratchBuffer* reply, u32 timeout_ms) {
    if(!rd_net_is_enabled()) return false;

    const RDDatum* servers =
        rd_datum_get_array(rd_settings_root(), "pdb.servers");

    RDScratchBuffer* url_buf = rd_scratch_create();
    bool ok = false;

    if(rd_datum_is_empty(servers)) {
        ok = _pdb_fetch(PDB_SYMBOL_SERVER_DEFAULT, pdb_name, guid, age, url_buf,
                        reply, timeout_ms);
    }
    else {
        const RDDatum* server_datum;
        rd_datum_each(server_datum, servers) {
            const char* server = rd_datum_to_str(server_datum);
            if(!server || !*server) continue;

            rd_scratch_clear(url_buf);
            ok = _pdb_fetch(server, pdb_name, guid, age, url_buf, reply,
                            timeout_ms);

            if(ok) break;

            rd_scratch_clear(reply);
        }
    }

    rd_scratch_destroy(url_buf);
    return ok;
}
