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

    return !rd_reader_has_error(r);
}

bool msvc_eh_read_funcinfo(RDReader* r, EHFuncInfo* v) {
    rd_reader_read_le32(r, &v->magicAndBbtFlags);
    rd_reader_read_le32(r, &v->maxState);
    rd_reader_read_le32(r, &v->pUnwindMap);
    rd_reader_read_le32(r, &v->nTryBlocks);
    rd_reader_read_le32(r, &v->pTryBlockMap);
    rd_reader_read_le32(r, &v->nIPMapEntries);
    rd_reader_read_le32(r, &v->pIPtoStateMap);
    return !rd_reader_has_error(r);
}

bool msvc_eh_read_unwindmapentry(RDReader* r, EHUnwindMapEntry* v) {
    rd_reader_read_le32(r, (u32*)&v->toState);
    rd_reader_read_le32(r, &v->action);
    return !rd_reader_has_error(r);
}

bool msvc_eh_read_tryblockmapentry(RDReader* r, EHTryBlockMapEntry* v) {
    rd_reader_read_le32(r, (u32*)&v->tryLow);
    rd_reader_read_le32(r, (u32*)&v->tryHigh);
    rd_reader_read_le32(r, (u32*)&v->catchHigh);
    rd_reader_read_le32(r, &v->nCatches);
    rd_reader_read_le32(r, &v->pHandlerArray);
    return !rd_reader_has_error(r);
}

bool msvc_eh_read_handlertype(RDReader* r, EHHandlerType* v) {
    rd_reader_read_le32(r, &v->adjectives);
    rd_reader_read_le32(r, &v->pType);
    rd_reader_read_le32(r, (u32*)&v->dispCatchObj);
    rd_reader_read_le32(r, &v->addressOfHandler);
    return !rd_reader_has_error(r);
}
