#ifndef EXT_H
#define EXT_H

#define start_f(statepath, restorepath, changepath) __declspec(dllexport) unsigned int start(char *statepath, char *restorepath, char *changepath)

#endif
