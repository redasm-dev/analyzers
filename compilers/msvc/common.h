#pragma once

#include <redasm/redasm.h>

bool msvc_rtti_segment_ok(RDContext* ctx, RDAddress address);
bool msvc_rtti_segment_exec_ok(RDContext* ctx, RDAddress address);
bool msvc_rtti_segment_fits(RDContext* ctx, RDAddress address, usize n);
bool msvc_rtti_is_typedescriptor_valid(const char* s, usize n);
bool msvc_rtti_addressslice_contains(RDAddressSlice locators,
                                     RDAddress address);

const char* msvc_rtti_classtag_from_typedescriptor(RDReader* r, RDAddress td_va,
                                                   bool is64);
