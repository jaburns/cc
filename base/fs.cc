#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

konst usize FS_PATH_BUFFER_SIZE = 4096;
global thread_local char g_fs_path_buffer[FS_PATH_BUFFER_SIZE];

void fs_load_path_buffer(Str str) {
    Assert(str.count < FS_PATH_BUFFER_SIZE - 1);
    MemCopy(g_fs_path_buffer, str.elems, str.count);
    g_fs_path_buffer[str.count] = '\0';
}

// -----------------------------------------------------------------------------

bool fs_file_exists(Str path) {
    fs_load_path_buffer(path);
    FILE* file = fopen(g_fs_path_buffer, "rb");
    if (file == nullptr) return false;
    fclose(file);
    return true;
}

void fs_write_file_bytes(Str path, Slice<u8> u8s) {
    fs_load_path_buffer(path);
    FILE* file = fopen(g_fs_path_buffer, "wb");
    AssertM(file, "failed to open file: %s", g_fs_path_buffer);
    fwrite(u8s.elems, u8s.count, 1, file);
    fclose(file);
}

void fs_append_file_bytes(Str path, Slice<u8> u8s) {
    fs_load_path_buffer(path);
    FILE* file = fopen(g_fs_path_buffer, "ab");
    AssertM(file, "failed to open file: %s", g_fs_path_buffer);
    fwrite(u8s.elems, u8s.count, 1, file);
    fclose(file);
}

void fs_remove_file_if_exists(Str path) {
    fs_load_path_buffer(path);
    int rc = remove(g_fs_path_buffer);
    AssertM(rc == 0 || errno == ENOENT, "failed to remove file: %s", g_fs_path_buffer);
}

Slice<u8> fs_read_file_bytes(Arena* arena, Str path) {
    fs_load_path_buffer(path);

    FILE* file = fopen(g_fs_path_buffer, "rb");
    AssertM(file, "failed to open file: %s", g_fs_path_buffer);

    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    arena->align(32);
    Slice<u8> content = arena->push_many<u8>(file_size);
    AssertM(fread(content.elems, 1, file_size, file) == file_size || !ferror(file), "failed to read file: %s", g_fs_path_buffer);
    fclose(file);

    return content;
}

void fs_mkdirp_for_file(Str path) {
    fs_load_path_buffer(path);
    for (char* p = strchr(g_fs_path_buffer + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(g_fs_path_buffer, 0755) == -1) {
            AssertM(errno == EEXIST, "failed to create directory");
        }
        *p = '/';
    }
}

// -----------------------------------------------------------------------------

void fread_ok(void* ptr, usize size, usize nitems, FILE* stream) {
    AssertM(fread(ptr, size, nitems, stream), "file read failed");
}

forall(T) T fread_ok_imm(FILE* stream) {
    T val;
    AssertM(fread(&val, sizeof(T), 1, stream), "file read failed");
    return val;
}

forall(T) void fwrite_imm(T val, FILE* stream) {
    fwrite(&val, sizeof(T), 1, stream);
}

// -----------------------------------------------------------------------------

FsFolderIter FsFolderIter::make(Str path) {
    fs_load_path_buffer(path);
    FsFolderIter ret = {};
    ret.dir = opendir(g_fs_path_buffer);
    if (ret.dir) {
        ret.next();
    } else {
        ret.done = true;
    }
    return ret;
}

void FsFolderIter::next() {
    errno = 0;
    entry = readdir(dir);
    if (entry) {
        item = Str::from_cstr(entry->d_name);
        item_is_dir = entry->d_type == DT_DIR;
        if (item.starts_with(".")) {
            next();
        }
    } else {
        Assert(errno == 0);
        Assert(closedir(dir) != -1);
        done = true;
    }
}

// -----------------------------------------------------------------------------
}  // namespace a
