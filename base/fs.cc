#include "inc.hh"
namespace {

Slice<u8> fs_read_file_bytes(Arena* arena, cchar* path) {
    FILE* file = fopen(path, "r");
    if (file == nullptr) Panic("Failed to open file: %s", path);

    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    arena->align();
    Slice<u8> content = arena->alloc_many<u8>(file_size);
    if (fread(content.elems, 1, file_size, file) != file_size && ferror(file)) Panic("Failed to read file: %s", path);
    fclose(file);

    return content;
}

}  // namespace