#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

bool fs_file_exists(Str path);
void fs_write_file_bytes(Str path, Slice<u8> u8s);
void fs_append_file_bytes(Str path, Slice<u8> u8s);
void fs_remove_file_if_exists(Str path);
Slice<u8> fs_read_file_bytes(Arena* arena, Str path);
void fs_mkdirp_for_file(Str file_path);

// -----------------------------------------------------------------------------

void fread_ok(void* ptr, usize size, usize nitems, FILE* stream);
forall(T) T fread_ok_imm(FILE* stream);
forall(T) void fwrite_imm(T val, FILE* stream);

// -----------------------------------------------------------------------------

class FsFolderIter {
    DIR* dir;
    dirent* entry;

  public:
    Str item;
    bool item_is_dir;
    bool done;

    func FsFolderIter make(Str path);
    void next();
};

// -----------------------------------------------------------------------------
}  // namespace a
