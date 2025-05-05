#include <windows.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <stdio.h>
#include <inttypes.h>

#include "util.h"
#include "error.h"

static enum error append_to_path(char *p, size_t psize, char *a, size_t asize) {
  if (psize + asize > MAX_PATH) {
    fputs("[ERROR] Path too large\n", stderr);
    return NFERR_TOOLARGEPATH;
  }

  strncat(p, a, MAX_PATH - psize);

  return SUCCESS;
}

static enum error append_wildcard_to_path(char *p, size_t psize) {
  if (psize + 2 > MAX_PATH) {
    fputs("[ERROR] Path too large\n", stderr);
    return NFERR_TOOLARGEPATH;
  }

  strncat(p, "\\*", MAX_PATH - psize);

  return SUCCESS;
}

enum error find_matching(char *statepath, size_t spsize, char *changepath, size_t cpsize, char *apppath, size_t apsize, char *restorepath, FILE *install, FILE *restore, struct modification_info_s *modification_info) {
  char changepathc[MAX_PATH + 1], statepathc[MAX_PATH + 1], apppathc[MAX_PATH + 1];

  strncpy(changepathc, changepath, MAX_PATH + 1);
  strncpy(statepathc, statepath, MAX_PATH + 1);
  strncpy(apppathc, apppath, MAX_PATH + 1);

  char changepathw[MAX_PATH + 1], statepathw[MAX_PATH + 1];

  strncpy(changepathw, changepathc, MAX_PATH + 1);
  strncpy(statepathw, statepathc, MAX_PATH + 1);

  enum error r;

  if ((r = append_wildcard_to_path(changepathw, cpsize + apsize)) != SUCCESS) {
    return r;
  }

  if ((r = append_wildcard_to_path(statepathw, spsize + apsize)) != SUCCESS) {
    return r;
  }

  HANDLE ch;
  WIN32_FIND_DATAA changeinfo;


  if ((ch = FindFirstFileA(changepathw, &changeinfo)) == INVALID_HANDLE_VALUE) {
    fputs("[FATAL ERROR] FindFirstFileA returned INVALID_HANDLE_VALUE\n", stderr);
    return ERR_FINDFIRSTFILEA;
  }

  do {
    if (strcmp(changeinfo.cFileName, ".") != 0 && strcmp(changeinfo.cFileName, "..") != 0) {
      HANDLE sh;
      WIN32_FIND_DATAA stateinfo;

      if ((sh = FindFirstFileA(statepathw, &stateinfo)) == INVALID_HANDLE_VALUE) {
	fputs("[FATAL ERROR] FindFirstFileA returned INVALID_HANDLE_VALUE\n", stderr);
	return ERR_FINDFIRSTFILEA;
      }

      _Bool file_matched = 0;

      do {
	if (strcmp(stateinfo.cFileName, ".") == 0 || strcmp(stateinfo.cFileName, "..") == 0) {
	  continue;
	}
	else {
	  if (strcmp(stateinfo.cFileName, changeinfo.cFileName) == 0) {
	    if (changeinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && stateinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
	      file_matched = 1;
	      size_t fnamesz = strlen(changeinfo.cFileName);

	      if ((r = append_to_path(apppathc, apsize, changeinfo.cFileName, fnamesz)) != SUCCESS) {
		return r;
	      }

	      if ((r = append_to_path(changepathc, cpsize, apppathc, apsize + fnamesz)) != SUCCESS) {
		return r;
	      }

	      if ((r = append_to_path(statepathc, spsize, apppathc, apsize + fnamesz)) != SUCCESS) {
		return r;
	      }

	      if ((r = find_matching(statepathc, spsize + apsize + fnamesz, changepathc, cpsize + apsize + fnamesz, apppathc, apsize + fnamesz, restorepath, install, restore, modification_info)) != SUCCESS) {
		return r;
	      }

	      strncpy(changepathc, changepath, MAX_PATH + 1);
	      strncpy(statepathc, statepath, MAX_PATH + 1);
	      strncpy(apppathc, apppath, MAX_PATH + 1);
	    }
	    else {
	      file_matched = 1;

	      if (fprintf(install, "xcopy /i /e /Y \"%s\\%s\" \"%s%s\\\"\n", statepathc, stateinfo.cFileName, restorepath, strcmp(apppathc, "\\") == 0? "": apppathc) < 0) {
		fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
		return ERR_FPRINTF;
	      }
	      modification_info->is_backedup++;

	      if (fprintf(install, "xcopy /Y \"%s\\%s\" \"%s\\\"\n", changepathc, changeinfo.cFileName, statepathc) < 0) {
		fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
		return ERR_FPRINTF;
	      }
	      modification_info->is_modified++;

	      if (fprintf(restore, "xcopy /Y \"%s%s\\%s\" \"%s\\\"\n", restorepath, strcmp(apppathc, "\\") == 0? "": apppathc, stateinfo.cFileName, statepathc) < 0) {
		fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
		return ERR_FPRINTF;
	      }
	      modification_info->rs_modified++;
	    }
	  }
	}
      } while (FindNextFileA(sh, &stateinfo) != 0);

      if (GetLastError() != ERROR_NO_MORE_FILES) {
	FindClose(sh);

	fputs("[FATAL ERROR] GetLastError did not return ERROR_NO_MORE_FILES; FindNextFileA likely failed\n", stderr);

	return ERR_FINDNEXTFILEA;
      }

      FindClose(sh);

      if (file_matched == 0) {
	if (changeinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
	  if (fputs("echo Y | ", restore) < 0) {
	    fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
	    return ERR_FPUTS;
	  }
	}

	if (fprintf(restore, "del \"%s\\%s\"\n", statepathc, changeinfo.cFileName) < 0) {
	  fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
	  return ERR_FPRINTF;
	}
	modification_info->rs_deleted++;

	if (changeinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
	  if (fprintf(restore, "rmdir \"%s\\%s\"\n", statepathc, changeinfo.cFileName) < 0) {
	    fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
	    return ERR_FPRINTF;
	  }

	  modification_info->rs_modified++;
	}

	else {
	  if (fputs("echo F | ", install) < 0) {
	    fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
	    return ERR_FPUTS;
	  }
	}

	if (fprintf(install, "xcopy /Y /i /e \"%s\\%s\" \"%s\\%s\"\n", changepathc, changeinfo.cFileName, statepathc, changeinfo.cFileName) < 0) {
	  fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
	  return ERR_FPRINTF;
	}
	modification_info->is_added++;
      }
    }

  } while (FindNextFileA(ch, &changeinfo) != 0);

  if (GetLastError() != ERROR_NO_MORE_FILES) {
    FindClose(ch);

    fputs("[FATAL ERROR] GetLastError did not return ERROR_NO_MORE_FILES; FindNextFileA likely failed\n", stderr);

    return ERR_FINDNEXTFILEA;
  }

  FindClose(ch);

  return SUCCESS;
}
