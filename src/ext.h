#ifndef EXT_H
#define EXT_H

enum action {
  SKIP_NONE,
  SKIP_DEFAULT,
  SKIP_CREATESCRIPTS,
  SKIP_EXTSTART
};

enum ext_error {
  EXT_SUCCESS,
  EXTERR_FATAL,
  EXTERR
};

#define init_f(statepath, restorepath, changepath, ext_err) __declspec(dllexport) enum action init(char *statepath, char *restorepath, char *changepath, enum ext_error *ext_err)
#define start_f(statepath, restorepath, changepath, ext_err) __declspec(dllexport) unsigned int start(char *statepath, char *restorepath, char *changepath, enum ext_error *ext_err)

#endif
