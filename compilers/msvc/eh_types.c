#include "eh_types.h"

bool msvc_eh_read_catchabletype(RDReader* r, EHCatchableType* v) {
    rd_reader_read_le32(r, &v->properties);
    rd_reader_read_le32(r, &v->pType);
    rd_reader_read_le32(r, (u32*)&v->where_mdisp);
    rd_reader_read_le32(r, (u32*)&v->where_pdisp);
    rd_reader_read_le32(r, (u32*)&v->where_vdisp);
    rd_reader_read_le32(r, &v->sizeOrOffset);
    rd_reader_read_le32(r, &v->copyFunction);

    return !rd_reader_has_error(r);
}

bool msvc_eh_read_catchabletypearrayheader(RDReader* r,
                                           EHCatchableTypeArrayHeader* v) {
    rd_reader_read_le32(r, &v->nCatchableTypes);
    return !rd_reader_has_error(r);
}

bool msvc_eh_read_throwinfo(RDReader* r, EHThrowInfo* v) {
    rd_reader_read_le32(r, &v->attributes);
    rd_reader_read_le32(r, &v->pmfnUnwind);
    rd_reader_read_le32(r, &v->pForwardCompat);
    rd_reader_read_le32(r, &v->pCatchableTypeArray);

    return !rd_reader_has_error(r);
}
