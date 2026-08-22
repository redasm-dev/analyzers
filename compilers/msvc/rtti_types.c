#include "rtti_types.h"

bool msvc_rtti_read_completeobjectlocator(RDReader* r,
                                          RTTICompleteObjectLocator* v) {
    if(!rd_reader_read_le32(r, &v->signature)) return false;
    if(v->signature > MSVC_RTTI_SIGNATURE_V1) return false;

    rd_reader_read_le32(r, &v->offset);
    rd_reader_read_le32(r, &v->cdOffset);
    rd_reader_read_le32(r, &v->pTypeDescriptor);
    rd_reader_read_le32(r, &v->pClassDescriptor);

    if(!rd_reader_has_error(r) && v->signature == MSVC_RTTI_SIGNATURE_V1) {
        rd_reader_read_le32(r, &v->pSelf);
        return !rd_reader_has_error(r) && v->pSelf;
    }

    v->pSelf = 0;
    return !rd_reader_has_error(r);
}

bool msvc_rtti_read_classhierarchydescriptor(RDReader* r,
                                             RTTIClassHierarchyDescriptor* v) {
    rd_reader_read_le32(r, &v->signature);
    rd_reader_read_le32(r, &v->attributes);
    rd_reader_read_le32(r, &v->numBaseClasses);
    rd_reader_read_le32(r, &v->pBaseClassArray);
    return !rd_reader_has_error(r);
}

bool msvc_rtti_read_baseclassdescriptor(RDReader* r,
                                        RTTIBaseClassDescriptor* v) {
    rd_reader_read_le32(r, &v->pTypeDescriptor);
    rd_reader_read_le32(r, &v->numContainedBases);
    rd_reader_read_le32(r, (u32*)&v->where_mdisp);
    rd_reader_read_le32(r, (u32*)&v->where_pdisp);
    rd_reader_read_le32(r, (u32*)&v->where_vdisp);
    rd_reader_read_le32(r, &v->attributes);

    if(!rd_reader_has_error(r) && (v->attributes & MSVC_RTTI_BCD_HASCHD))
        rd_reader_read_le32(r, &v->pClassDescriptor);
    else
        v->pClassDescriptor = 0;

    return !rd_reader_has_error(r);
}

bool msvc_rtti_read_typedescriptor32(RDReader* r, RTTITypeDescriptor32* v) {
    rd_reader_read_le32(r, &v->pVFTable);
    rd_reader_read_le32(r, &v->spare);
    return !rd_reader_has_error(r) && !v->spare;
}

bool msvc_rtti_read_typedescriptor64(RDReader* r, RTTITypeDescriptor64* v) {
    rd_reader_read_le64(r, &v->pVFTable);
    rd_reader_read_le64(r, &v->spare);
    return !rd_reader_has_error(r) && !v->spare;
}
