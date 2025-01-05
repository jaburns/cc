#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void        watch_fs_create(const char** paths, unsigned int path_count);
const char* watch_fs_check_file_changed(void);
void        watch_fs_destroy(void);

#ifdef __cplusplus
}
#endif