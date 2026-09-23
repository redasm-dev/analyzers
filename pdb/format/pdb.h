#pragma once

#include <redasm/redasm.h>

#define PDB_SUPERBLOCK_MAGIC_LENGTH 32

typedef enum {
    PDB_STREAM_ROOT = 0,
    PDB_STREAM_INFO,
    PDB_STREAM_TPI,
    PDB_STREAM_DBI,
    PDB_STREAM_IPI,
} PDBStreamType;

typedef struct PDBSuperBlock {
    char FileMagic[PDB_SUPERBLOCK_MAGIC_LENGTH];
    u32 PageSize;
    u32 FlagPage;
    u32 NumPages;
    u32 RootSize;
    u32 Reserved;
    u32 RootPageIndexes;
} PDBSuperBlock;

typedef struct PDBStream {
    u8* data;
    u32 size;
} PDBStream;

typedef struct PDBInfoHeader {
    u32 Version;
    u32 Signature;
    u32 Age;
    u8 Guid[16];
} PDBInfoHeader;

typedef struct PDBFile {
    RDReader* r;
    PDBSuperBlock super_block;
    u32 n_streams;
    u32* sizes;
    u32** stream_pages;  // stream_pages[i] = page list for stream i
    u32* stream_n_pages; // stream_n_pages[i] = page count for stream i

    PDBInfoHeader info;
    char* guid;
} PDBFile;

typedef struct PDBCvInfo70 {
    u32 CvSignature;
    u32 GuidData1;   // Little-Endian (4 byte)
    u16 GuidData2;   // Little-Endian (2 byte)
    u16 GuidData3;   // Little-Endian (2 byte)
    u8 GuidData4[8]; // Big-Endian (8 byte)
    u32 Age;
} PDBCvInfo70;

// superblock
bool pdb_read_superblock(RDReader* r, PDBSuperBlock* out);
bool pdb_check_pagesize(const PDBSuperBlock* pdb);
bool pdb_check_numblocks(const RDReader* r, const PDBSuperBlock* pdb);
bool pdb_check_rootsize(const PDBSuperBlock* pdb);
bool pdb_check_rootpageindex(const RDReader* r, const PDBSuperBlock* pdb);

// stream I/O
bool pdb_read_stream(RDReader* r, const PDBSuperBlock* sb, const u32* pages,
                     u32 n_pages, PDBStream* out);
bool pdb_read_info_header(const PDBStream* s, PDBInfoHeader* out);
void pdb_stream_destroy(PDBStream* s);

// PDB file
bool pdb_open(const char* path, PDBFile* out);
bool pdb_open_data(const void* data, usize size, PDBFile* out);
bool pdb_verify(PDBFile* pdb, const char* expected_guid, u32 expected_age);
bool pdb_read_stream_by_index(PDBFile* pdb, u32 idx, PDBStream* out);
void pdb_close(PDBFile* pdb);

// PDB header
bool pdb_read_cv_info_70(RDReader* r, PDBCvInfo70* pdb);
