#ifndef ERROR_H
#define ERROR_H

enum error {
  SUCCESS = 0,
  ERR_FINDFIRSTFILEA,
  NFERR_TOOLARGEPATH,
  ERR_FGETC,
  ERR_FPUTS,
  ERR_SCANF,
  ERR_FOPEN,
  ERR_FPRINTF,
  ERR_NOTAVALIDPATH,
  ERR_GETFILEATTRIBUTESA,
  ERR_FINDNEXTFILEA,
  ERR_GETFULLPATHNAMEA,
  ERR_LOADLIBRARYA,
  ERR_GETPROCADDRESS,
  ERR_MALLOC,
  ERR_FERROR
};

#endif
