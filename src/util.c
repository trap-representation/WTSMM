#include <windows.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

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

static enum error cmp(char *p1, size_t p1size, char *p2, size_t p2size, char *f, size_t fsize, _Bool *same) {
  enum error r;

  char p1c[MAX_PATH + 1];
  char p2c[MAX_PATH + 1];

  strncpy(p1c, p1, MAX_PATH + 1);
  strncpy(p2c, p2, MAX_PATH + 1);

  if ((r = append_to_path(p1c, p1size, f, fsize)) != SUCCESS) {
    return r;
  }

  if ((r = append_to_path(p2c, p2size, f, fsize)) != SUCCESS) {
    return r;
  }

  FILE *p1f = fopen(p1c, "rb");
  if (p1f == NULL) {
    fputs("[FATAL ERROR] fopen returned NULL\n", stderr);
    return ERR_FOPEN;
  }

  FILE *p2f = fopen(p2c, "rb");
  if (p2f == NULL) {
    fputs("[FATAL ERROR] fopen returned NULL\n", stderr);
    fclose(p1f);
    return ERR_FOPEN;
  }

  char *p1r = malloc(MAX_CMP_BUF_LEN);
  if (p1r == NULL) {
    fputs("[FATAL ERROR] malloc returned NULL\n", stderr);
    fclose(p2f);
    fclose(p1f);
    return ERR_MALLOC;
  }

  char *p2r = malloc(MAX_CMP_BUF_LEN);
  if (p2r == NULL) {
    fputs("[FATAL ERROR] malloc returned NULL\n", stderr);
    free(p1r);
    fclose(p2f);
    fclose(p1f);
    return ERR_MALLOC;
  }

  size_t r1, r2;

  while((r1 = fread(p1r, 1, MAX_CMP_BUF_LEN, p1f)) != 0) {
    if (ferror(p1f)) {
      fputs("[FATAL ERROR] ferror returned a non-zero value\n", stderr);
      free(p2r);
      free(p1r);
      fclose(p2f);
      fclose(p1f);
      return ERR_FERROR;
    }

    r2 = fread(p2r, 1, MAX_CMP_BUF_LEN, p2f);

    if (ferror(p2f)) {
      fputs("[FATAL ERROR] ferror returned a non-zero value\n", stderr);
      free(p2r);
      free(p1r);
      fclose(p2f);
      fclose(p1f);
      return ERR_FERROR;
    }

    if (r1 != r2) {
      *same = 0;
      free(p2r);
      free(p1r);
      fclose(p2f);
      fclose(p1f);
      return SUCCESS;
    }

    if (memcmp(p1r, p2r, r1) != 0) {
      *same = 0;
      free(p2r);
      free(p1r);
      fclose(p2f);
      fclose(p1f);
      return SUCCESS;
    }
  }

  if (fread(p2r, 1, MAX_CMP_BUF_LEN, p2f) != 0) {
    if (ferror(p2f)) {
      fputs("[FATAL ERROR] ferror returned a non-zero value\n", stderr);
      free(p2r);
      free(p1r);
      fclose(p2f);
      fclose(p1f);
      return ERR_FERROR;
    }

    *same = 0;

    free(p2r);
    free(p1r);
    fclose(p2f);
    fclose(p1f);

    return SUCCESS;
  }

  *same = 1;

  return SUCCESS;
}

enum error find_matching(char *statepath, size_t spsize, char *changepath, size_t cpsize, char *apppath, size_t apsize, char *restorepath, FILE *install, FILE *restore, struct modification_info_s *modification_info, _Bool cmp_files) {
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

	      if ((r = find_matching(statepathc, spsize + apsize + fnamesz, changepathc, cpsize + apsize + fnamesz, apppathc, apsize + fnamesz, restorepath, install, restore, modification_info, cmp_files)) != SUCCESS) {
		return r;
	      }

	      strncpy(changepathc, changepath, MAX_PATH + 1);
	      strncpy(statepathc, statepath, MAX_PATH + 1);
	      strncpy(apppathc, apppath, MAX_PATH + 1);
	    }
	    else {
	      file_matched = 1;

	      _Bool same;

	      if (cmp_files) {
		if ((r = cmp(statepathc, spsize, changepathc, cpsize, stateinfo.cFileName, strlen(stateinfo.cFileName), &same)) != SUCCESS) {
		  return r;
		}
	      }

	      if ((cmp_files && !same) || !cmp_files) {

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
