#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void        watchfs_create(const char** paths, unsigned int path_count);
const char* watchfs_check_file_changed(void);
void        watchfs_destroy(void);

#ifdef __cplusplus
}
#endif