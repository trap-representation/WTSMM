#ifndef EXT_H
#define EXT_H

enum process {
  SKIP_NONE,
  SKIP_DEFAULT,
  SKIP_CREATESCRIPTS,
  SKIP_EXTSTART
};

#define init_f(statepath, restorepath, changepath) __declspec(dllexport) enum process init(char *statepath, char *restorepath, char *changepath)
#define start_f(statepath, restorepath, changepath) __declspec(dllexport) unsigned int start(char *statepath, char *restorepath, char *changepath)

#endif
