#ifndef UTIL_H
#define UTIL_H

#define MAX_CMP_BUF_LEN 1048576

struct modification_info_s {
  uintmax_t is_backedup, is_modified, is_added, rs_modified, rs_deleted;
};

enum error find_matching(char *statepath, size_t spsize, char *changepath, size_t cpsize, char *apppath, size_t apsize, char *restorepath, FILE *install, FILE *restore, struct modification_info_s *modification_info, _Bool cmp_files);

#endif
