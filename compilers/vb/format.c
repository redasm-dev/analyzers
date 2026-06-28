#include "format.h"

static bool _pe_vb_read_lcid(RDReader* r, PELCID* v) {
    rd_reader_read_le32(r, v);
    return !rd_reader_has_error(r);
}

bool pe_vb_read_guid(RDReader* r, PEGUID* v) {
    rd_reader_read_le32(r, &v->data1);
    rd_reader_read_le16(r, &v->data2);
    rd_reader_read_le16(r, &v->data3);
    rd_reader_read(r, v->data4, sizeof(v->data4));

    return !rd_reader_has_error(r);
}

bool pe_vb_read_header(RDReader* r, PEVBHeader* v) {
    rd_reader_read(r, v->szVbMagic, sizeof(v->szVbMagic));
    rd_reader_read_le16(r, &v->wRuntimeBuild);
    rd_reader_read(r, v->szLangDll, sizeof(v->szLangDll));
    rd_reader_read(r, v->szSecLangDll, sizeof(v->szSecLangDll));
    rd_reader_read_le16(r, &v->wRuntimeRevision);
    rd_reader_read_le32(r, &v->dwLCID);
    rd_reader_read_le32(r, &v->dwSecLCID);
    rd_reader_read_le32(r, &v->lpSubMain);
    rd_reader_read_le32(r, &v->lpProjectData);
    rd_reader_read_le32(r, &v->fMdlIntCtls);
    rd_reader_read_le32(r, &v->fMdlIntCtls2);
    rd_reader_read_le32(r, &v->dwThreadFlags);
    rd_reader_read_le32(r, &v->dwThreadCount);
    rd_reader_read_le16(r, &v->wFormCount);
    rd_reader_read_le16(r, &v->wExternalCount);
    rd_reader_read_le32(r, &v->dwThunkCount);
    rd_reader_read_le32(r, &v->lpGuiTable);
    rd_reader_read_le32(r, &v->lpExternalCompTable);
    rd_reader_read_le32(r, &v->lpComRegisterData);
    rd_reader_read_le32(r, &v->bszProjectDescription);
    rd_reader_read_le32(r, &v->bszProjectExeName);
    rd_reader_read_le32(r, &v->bszProjectHelpFile);
    rd_reader_read_le32(r, &v->bszProjectName);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_project_info(RDReader* r, PEVBProjectInfo* v) {
    rd_reader_read_le32(r, &v->dwVersion);
    rd_reader_read_le32(r, &v->lpObjectTable);
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->lpCodeStart);
    rd_reader_read_le32(r, &v->lpCodeEnd);
    rd_reader_read_le32(r, &v->dwDataSize);
    rd_reader_read_le32(r, &v->lpThreadSpace);
    rd_reader_read_le32(r, &v->lpVbaSeh);
    rd_reader_read_le32(r, &v->lpNativeCode);
    rd_reader_read(r, &v->szPathInformation, sizeof(v->szPathInformation));
    rd_reader_read_le32(r, &v->lpExternalTable);
    rd_reader_read_le32(r, &v->dwExternalCount);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_gui_table(RDReader* r, PEVBGuiTable* v) {
    rd_reader_read_le32(r, &v->lpSectionHeader);
    rd_reader_read(r, &v->dwReserved, sizeof(v->dwReserved));
    rd_reader_read_le32(r, &v->dwFormSize);
    rd_reader_read_le32(r, &v->dwReserved1);
    rd_reader_read_le32(r, &v->lpFormData);
    rd_reader_read_le32(r, &v->dwReserved2);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_object_table(RDReader* r, PEVBObjectTable* v) {
    rd_reader_read_le32(r, &v->lpHeapLink);
    rd_reader_read_le32(r, &v->lpExecProj);
    rd_reader_read_le32(r, &v->lpObjectTreeInfo);
    rd_reader_read_le32(r, &v->dwReserved);
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->lpProjectObject);
    pe_vb_read_guid(r, &v->uuidObject);
    rd_reader_read_le16(r, &v->fCompileState);
    rd_reader_read_le16(r, &v->wTotalObjects);
    rd_reader_read_le16(r, &v->wCompiledObjects);
    rd_reader_read_le16(r, &v->wObjectsInUse);
    rd_reader_read_le32(r, &v->lpPubObjArray);
    rd_reader_read_le32(r, &v->fIdeFlag);
    rd_reader_read_le32(r, &v->lpIdeData);
    rd_reader_read_le32(r, &v->lpIdeData2);
    rd_reader_read_le32(r, &v->lpszProjectName);
    _pe_vb_read_lcid(r, &v->dwLcid);
    _pe_vb_read_lcid(r, &v->dwLcid2);
    rd_reader_read_le32(r, &v->lpIdeData3);
    rd_reader_read_le32(r, &v->dwIdentifier);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_object_tree(RDReader* r, PEVBObjectTree* v) {
    rd_reader_read_le32(r, &v->lpHeapLink);
    rd_reader_read_le32(r, &v->lpObjectTable);
    rd_reader_read_le32(r, &v->dwReserved);
    rd_reader_read_le32(r, &v->dwUnused);
    rd_reader_read_le32(r, &v->lpFormList);
    rd_reader_read_le32(r, &v->dwUnused2);
    rd_reader_read_le32(r, &v->szProjectDescription);
    rd_reader_read_le32(r, &v->szProjectHelpFile);
    rd_reader_read_le32(r, &v->dwReserved2);
    rd_reader_read_le32(r, &v->dwHelpContextId);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_public_object_descriptor(RDReader* r,
                                         PEVBPublicObjectDescriptor* v) {
    rd_reader_read_le32(r, &v->lpObjectInfo);
    rd_reader_read_le32(r, &v->dwReserved);
    rd_reader_read_le32(r, &v->lpPublicBytes);
    rd_reader_read_le32(r, &v->lpStaticBytes);
    rd_reader_read_le32(r, &v->lpModulePublic);
    rd_reader_read_le32(r, &v->lpModuleStatic);
    rd_reader_read_le32(r, &v->lpszObjectName);
    rd_reader_read_le32(r, &v->dwMethodCount);
    rd_reader_read_le32(r, &v->lpMethodNames);
    rd_reader_read_le32(r, &v->bStaticVars);
    rd_reader_read_le32(r, &v->fObjectType);
    rd_reader_read_le32(r, &v->dwNull);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_object_info(RDReader* r, PEVBObjectInfo* v) {
    rd_reader_read_le16(r, &v->wRefCount);
    rd_reader_read_le16(r, &v->wObjectIndex);
    rd_reader_read_le32(r, &v->lpObjectTable);
    rd_reader_read_le32(r, &v->lpIdeData);
    rd_reader_read_le32(r, &v->lpPrivateObject);
    rd_reader_read_le32(r, &v->dwReserved);
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->lpObject);
    rd_reader_read_le32(r, &v->lpProjectData);
    rd_reader_read_le16(r, &v->wMethodCount);
    rd_reader_read_le16(r, &v->wMethodCount2);
    rd_reader_read_le32(r, &v->lpMethods);
    rd_reader_read_le16(r, &v->wConstants);
    rd_reader_read_le16(r, &v->wMaxConstants);
    rd_reader_read_le32(r, &v->lpIdeData2);
    rd_reader_read_le32(r, &v->lpIdeData3);
    rd_reader_read_le32(r, &v->lpConstants);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_object_info_optional(RDReader* r, PEVBObjectInfoOptional* v) {
    pe_vb_read_object_info(r, &v->base);
    rd_reader_read_le32(r, &v->dwObjectGuids);
    rd_reader_read_le32(r, &v->lpObjectGuid);
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->lpuuidObjectTypes);
    rd_reader_read_le32(r, &v->dwObjectTypeGuids);
    rd_reader_read_le32(r, &v->lpControls2);
    rd_reader_read_le32(r, &v->dwNull2);
    rd_reader_read_le32(r, &v->lpObjectGuid2);
    rd_reader_read_le32(r, &v->dwControlCount);
    rd_reader_read_le32(r, &v->lpControls);
    rd_reader_read_le16(r, &v->wEventCount);
    rd_reader_read_le16(r, &v->wPCodeCount);
    rd_reader_read_le16(r, &v->bWInitializeEvent);
    rd_reader_read_le16(r, &v->bWTerminateEvent);
    rd_reader_read_le32(r, &v->lpEvents);
    rd_reader_read_le32(r, &v->lpBasicClassObject);
    rd_reader_read_le32(r, &v->dwNull3);
    rd_reader_read_le32(r, &v->lpIdeData);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_control_info(RDReader* r, PEVBControlInfo* v) {
    rd_reader_read_le32(r, &v->fControlType);
    rd_reader_read_le16(r, &v->wEventCount);
    rd_reader_read_le16(r, &v->bWEventsOffset);
    rd_reader_read_le32(r, &v->lpGuid);
    rd_reader_read_le32(r, &v->dwIndex);
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->dwNull2);
    rd_reader_read_le32(r, &v->lpEventInfo);
    rd_reader_read_le32(r, &v->lpIdeData);
    rd_reader_read_le32(r, &v->lpszName);
    rd_reader_read_le32(r, &v->dwIndexCopy);

    return !rd_reader_has_error(r);
}

bool pe_vb_read_event_info(RDReader* r, PEVBEventInfo* v) {
    rd_reader_read_le32(r, &v->dwNull);
    rd_reader_read_le32(r, &v->lpControlsList);
    rd_reader_read_le32(r, &v->lpFormDescriptor);
    rd_reader_read_le32(r, &v->lpEVENT_SINK_QueryInterface);
    rd_reader_read_le32(r, &v->lpEVENT_SINK_AddRef);
    rd_reader_read_le32(r, &v->lpEVENT_SINK_Release);

    return !rd_reader_has_error(r);
}
