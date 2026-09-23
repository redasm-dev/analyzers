#pragma once

#include <redasm/redasm.h>

bool pdb_fetch_from_server(const char* pdb_name, const char* guid, u32 age,
                           RDScratchBuffer* reply, u32 timeout_ms);
