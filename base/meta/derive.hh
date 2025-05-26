#pragma once
#include "../inc.hh"
namespace a {
// -----------------------------------------------------------------------------

struct DeriveStructField {
    Str type;
    Str name;
    // List<Str> tags;
};

struct DeriveStructInfo {
    Str name;
    List<DeriveStructField> fields;
    Str target_hh_path;
    Str target_cc_path;
    bool has_post_deserialize_method;
};

typedef void (*DeriveHandler)(DeriveStructInfo* info);

void derive_run(Str root_dir);

// -----------------------------------------------------------------------------
}  // namespace a
