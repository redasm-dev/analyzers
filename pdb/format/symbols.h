#pragma once

#include "dbi.h"

bool pdb_apply_symbols(RDContext* ctx, PDBFile* pdb, u16 sym_stream_idx,
                       const PDBSectionHeaderList* sections);
