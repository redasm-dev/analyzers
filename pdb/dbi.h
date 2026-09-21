#pragma once

#include "pdb.h"

#define PDB_DBI_DBG_HEADER_SIZE 22
#define PDB_SECTION_HEADER_SIZE 40

typedef struct PDBDbiHeader {
    i32 VerSig;
    u32 VerHdr;
    u32 Age;
    u16 GSI;
    u16 Build;
    u16 PSI;
    u16 PdbDllVer;
    u16 SymRec; // symbol record stream index
    u16 PdbDllRbld;
    i32 ModInfoSize;
    i32 SecContrSize;
    i32 SecMapSize;
    i32 SrcInfoSize;
    i32 TypeSrvSize;
    u32 MFCIdx;
    i32 DbgHdrSize;
    i32 ECSize;
    u16 Flags;
    u16 Machine;
    // u32 pad
} PDBDbiHeader;

typedef struct PDBDbiDbgHeader {
    u16 FPO;
    u16 Exception;
    u16 Fixup;
    u16 OmapToSrc;
    u16 OmapFromSrc;
    u16 SectionHdr; // stream index for section headers
    u16 TokenRidMap;
    u16 Xdata;
    u16 Pdata;
    u16 NewFPO;
    u16 SectionHdrOrig;
} PDBDbiDbgHeader;

typedef struct PDBSectionHeader {
    char Name[8];
    u32 VirtualSize;
    u32 VirtualAddress;
    u32 SizeOfRawData;
    u32 PointerToRawData;
} PDBSectionHeader;

typedef struct PDBSectionHeaderList {
    PDBSectionHeader* headers;
    u32 count;
} PDBSectionHeaderList;

bool pdb_read_dbi_header(const PDBStream* s, PDBDbiHeader* out);
bool pdb_read_dbi_dbg_header(const PDBStream* s, const PDBDbiHeader* dbi,
                             PDBDbiDbgHeader* out);
bool pdb_read_section_headers(PDBFile* pdb, u16 stream_idx,
                              PDBSectionHeaderList* out);
void pdb_section_header_list_destroy(PDBSectionHeaderList* list);
bool pdb_section_va(const RDContext* ctx, const PDBSectionHeaderList* list,
                    u16 section, u32 offset, RDAddress* out);
