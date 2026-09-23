#include "request.h"

#define PDB_SYMBOL_SERVER "https://msdl.microsoft.com/download/symbols"

bool pdb_fetch_from_server(const char* pdb_name, const char* guid, u32 age,
                           RDScratchBuffer* reply, u32 timeout_ms) {
    if(!rd_net_is_enabled()) return false;

    RDScratchBuffer* url = rd_scratch_create();
    rd_scratch_puts(url, PDB_SYMBOL_SERVER);
    rd_scratch_putchar(url, '/');
    rd_scratch_puts(url, pdb_name);
    rd_scratch_putchar(url, '/');
    rd_scratch_puts(url, guid);
    rd_scratch_putf(url, "%X", age);
    rd_scratch_putchar(url, '/');
    rd_scratch_puts(url, pdb_name);

    const char* full_url = rd_scratch_data(url);
    RD_LOG_INFO("fetching %s", full_url);

    RDNetStatus st = rd_net_get(full_url, reply, timeout_ms);
    rd_scratch_destroy(url);

    if(!st.ok || st.code >= 400) {
        RD_LOG_WARN("fetch failed (%s)",
                    st.ok ? "HTTP error" : "transport error");
        return false;
    }

    return true;
}
