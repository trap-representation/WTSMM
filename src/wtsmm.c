#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <fileapi.h>
#include <ctype.h>
#include <inttypes.h>
#include <libloaderapi.h>

#include "util.h"
#include "error.h"
#include "ext.h"

#define MAX_PATHS "260"

static enum error verify_path_file(char *path) {
  DWORD atr;

  if ((atr = GetFileAttributesA(path)) == INVALID_FILE_ATTRIBUTES) {
    fputs("[FATAL ERROR] GetFileAttributesA returned INVALID_FILE_ATTRIBUTES\n", stderr);
    return ERR_GETFILEATTRIBUTESA;
  }
  else if (atr & FILE_ATTRIBUTE_DIRECTORY) {
    return ERR_NOTAVALIDPATH;
  }

  return SUCCESS;
}

static enum error verify_path_dir(char *path) {
  DWORD atr;

  if ((atr = GetFileAttributesA(path)) == INVALID_FILE_ATTRIBUTES) {
    fputs("[FATAL ERROR] GetFileAttributesA returned INVALID_FILE_ATTRIBUTES\n", stderr);
    return ERR_GETFILEATTRIBUTESA;
  }
  else if (atr & FILE_ATTRIBUTE_DIRECTORY) {
    if (path[strlen(path) - 1] == '\\') {
      fputs("[ERROR] Path cannot end with a '\\'\n", stderr);
      return ERR_NOTAVALIDPATH;
    }
    /* for (size_t i = 0; path[i] != '\0'; i++) {
      if (path[i] == '*') {
	fputs("[ERROR] Path cannot contain a wildcard character\n", stderr);
	return ERR_NOTAVALIDPATH;
      }
    }
    */
  }
  else {
    return ERR_NOTAVALIDPATH;
  }

  return SUCCESS;
}

int main(int argc, char *argv[]) {
  char statepath[MAX_PATH] = "";
  char changepath[MAX_PATH] = "";
  char restorepath[MAX_PATH] = "";
  char extensionpath[MAX_PATH] = "";

  char opt;

  struct done_s {
    char add_a_state[4];
    char add_a_change[4];
    char add_restorepath[4];
    char compare_files[4];
    char applied[4];
  } done;

  strncpy(done.add_a_state, "[ ]", sizeof(done.add_a_state));
  strncpy(done.add_a_change, "[ ]", sizeof(done.add_a_change));
  strncpy(done.add_restorepath, "[ ]", sizeof(done.add_restorepath));
  strncpy(done.applied, "[ ]", sizeof(done.applied));

  enum error r;

  while (1) {
    fprintf(stderr, "[INFO] Options:\n"
	    "[INFO] 1. Add a state (%s) %s\n"
	    "[INFO] 2. Add a change (%s) %s\n"
	    "[INFO] 3. Add restore path (%s) %s\n"
	    "[INFO] 4. Extension (%s)\n"
	    "[INFO] 5. Compare files (%s)\n"
	    "[INFO] 6. Apply %s\n"
	    "[INFO] 6. Quit\n", statepath, done.add_a_state, changepath, done.add_a_change, restorepath, done.add_restorepath, extensionpath, done.compare_files, done.applied);

    while (1) {
      if (scanf(" %c", &opt) != 1) {
	fputs("[FATAL ERROR] scanf could not set input item\n", stderr);
	return ERR_SCANF;
      }

      if (opt >= '1' && opt <= '7') {
	break;
      }

      fprintf(stderr, "[ERROR] Ignoring invalid option '%c'\n", opt);
    }

    switch(opt) {
    case '1':
      if (scanf(" %" MAX_PATHS "[^\n]", statepath) != 1) {
	fputs("[FATAL ERROR] scanf could not set input item\n", stderr);
	return ERR_SCANF;
      }

      if (verify_path_dir(statepath) != SUCCESS) {
	fputs("[ERROR] Not a valid directory\n", stderr);
	statepath[0] = '\0';
      }
      else {
	if ((r = GetFullPathNameA(statepath, MAX_PATH + 1, statepath, NULL)) > MAX_PATH + 1) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned a value greater than MAX_PATH; to fix this error, you will have to modify the source\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}
	else if (r == 0) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned 0\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}
	strncpy(done.add_a_state, "[X]", sizeof(done.add_a_state));
	strncpy(done.applied, "[ ]", sizeof(done.applied));
      }

      break;

    case '2':
      if (scanf(" %" MAX_PATHS "[^\n]", changepath) != 1) {
	fputs("[FATAL ERROR] scanf could not set input item\n", stderr);
	return ERR_SCANF;
      }

      if (verify_path_dir(changepath) != SUCCESS) {
	fputs("[ERROR] Not a valid directory\n", stderr);
	changepath[0] = '\0';
      }
      else {
	if ((r = GetFullPathNameA(changepath, MAX_PATH + 1, changepath, NULL)) > MAX_PATH + 1) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned a value greater than MAX_PATH; to fix this error, you will have to modify the source\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}
	else if (r == 0) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned 0\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}

	strncpy(done.add_a_change, "[X]", sizeof(done.add_a_change));
	strncpy(done.applied, "[ ]", sizeof(done.applied));
      }

      break;

    case '3':
      if (scanf(" %" MAX_PATHS "[^\n]", restorepath) != 1) {
	fputs("[FATAL ERROR] scanf could not set input item\n", stderr);
	return ERR_SCANF;
      }

      if (verify_path_dir(restorepath) != SUCCESS) {
	fputs("[ERROR] Not a valid directory\n", stderr);
	restorepath[0] = '\0';
      }
      else {
	if ((r = GetFullPathNameA(restorepath, MAX_PATH + 1, restorepath, NULL)) > MAX_PATH + 1) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned a value greater than MAX_PATH; to fix this error, you will have to modify the source\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}
	else if (r == 0) {
	  fputs("[FATAL ERROR] GetFullPathNameA returned 0\n", stderr);
	  return ERR_GETFULLPATHNAMEA;
	}

	strncpy(done.add_restorepath, "[X]", sizeof(done.add_restorepath));
	strncpy(done.applied, "[ ]", sizeof(done.applied));
      }

      break;

    case '4':
      if (scanf(" %" MAX_PATHS "[^\n]", extensionpath) != 1) {
	fputs("[FATAL ERROR] scanf could not set input item\n", stderr);
	return ERR_SCANF;
      }

      if (verify_path_file(extensionpath) != SUCCESS) {
	fputs("[ERROR] Not a valid directory\n", stderr);
	extensionpath[0] = '\0';
      }

      break;

    case '5':
      strncpy(done.compare_files, "[X]", sizeof(done.compare_files));
      break;

    case '6':
      {
	enum ext_error er = EXT_SUCCESS;

	if (done.add_a_state[1] == 'X' && done.add_a_change[1] == 'X' && done.add_restorepath[1] == 'X') {
	  enum action action_kind = SKIP_NONE;
	  r = 0;

	  if (extensionpath[0] != '\0') {
	    HMODULE libh;
	    if ((libh = LoadLibraryA(extensionpath)) == NULL) {
	      fputs("[FATAL ERROR] LoadLibraryA returned NULL\n", stderr);
	      return ERR_LOADLIBRARYA;
	    }

	    typedef enum action (*init_t)(char *, char *, char *, enum ext_err *);

	    init_t init_ptr;

	    if ((init_ptr = (init_t) GetProcAddress(libh, "init")) == NULL) {
	      fputs("[FATAL ERROR] GetProcAddress returned NULL\n", stderr);
	      FreeLibrary(libh);
	      return ERR_GETPROCADDRESS;
	    }

	    action_kind = init_ptr(statepath, changepath, restorepath, &er);

	    FreeLibrary(libh);
	  }

	  if (er == EXTERR_FATAL) {
	    return er;
	  }

	  if (action_kind != SKIP_DEFAULT) {
	    if (action_kind != SKIP_CREATESCRIPTS) {

	      char ispath[MAX_PATH + 1], rspath[MAX_PATH + 1];

	      size_t restorepathsz = strlen(restorepath);

	      if (restorepathsz + strlen("\\install.bat") > MAX_PATH) {
		fputs("[ERROR] Path too large\n", stderr);
		r = NFERR_TOOLARGEPATH;
	      }

	      if (restorepathsz + strlen("\\restore.bat") > MAX_PATH) {
		fputs("[ERROR] Path too large\n", stderr);
		r = NFERR_TOOLARGEPATH;
	      }

	      strncpy(ispath, restorepath, MAX_PATH + 1);

	      strncat(ispath, "\\install.bat", MAX_PATH - restorepathsz);

	      strncpy(rspath, restorepath, MAX_PATH + 1);

	      strncat(rspath, "\\restore.bat", MAX_PATH - restorepathsz);

	      FILE *install, *restore;

	      if ((install = fopen(ispath, "w")) == NULL) {
		fputs("[FATAL ERROR] fopen returned NULL\n", stderr);
		return ERR_FOPEN;
	      }

	      if ((restore = fopen(rspath, "w")) == NULL) {
		fputs("[FATAL ERROR] fopen returned NULL\n", stderr);
		fclose(install);
		return ERR_FOPEN;
	      }

	      struct modification_info_s modification_info = {0};

	      r = find_matching(statepath, strlen(statepath), changepath, strlen(changepath), "\\", 1, restorepath, install, restore, &modification_info, done.compare_files[1] == 'X'? 1: 0);


	      if (fprintf(install, "echo %ju files backed up from state, %ju files modified in state, %ju files added to state\n", modification_info.is_backedup, modification_info.is_modified, modification_info.is_added) < 0) {
		fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
		fclose(restore);
		fclose(install);
		return ERR_FPRINTF;
	      }

	      if (fprintf(restore, "echo %ju files modified in state, %ju files deleted from state\n", modification_info.rs_modified, modification_info.rs_deleted) < 0) {
		fputs("[FATAL ERROR] fprintf returned a negative value\n", stderr);
		fclose(restore);
		fclose(install);
		return ERR_FPRINTF;
	      }

	      fclose(restore);
	      fclose(install);

	      if (r != SUCCESS && r != NFERR_TOOLARGEPATH) {
		return r;
	      }
	    }

	    if (action_kind != SKIP_EXTSTART) {
	      if (extensionpath[0] != '\0') {
		HMODULE libh;
		if ((libh = LoadLibraryA(extensionpath)) == NULL) {
		  fputs("[FATAL ERROR] LoadLibraryA returned NULL\n", stderr);
		  return ERR_LOADLIBRARYA;
		}

		typedef unsigned int (*start_t)(char *, char *, char *, enum ext_err *);

		start_t start_ptr;

		if ((start_ptr = (start_t) GetProcAddress(libh, "start")) == NULL) {
		  fputs("[FATAL ERROR] GetProcAddress returned NULL\n", stderr);
		  FreeLibrary(libh);
		  return ERR_GETPROCADDRESS;
		}

		if ((r = start_ptr(statepath, changepath, restorepath, &er)) != 0) {
		  fputs("[ERROR] extension returned a non-zero value\n", stderr);
		}

		FreeLibrary(libh);
	      }
	    }
	  }

	  if (er == EXTERR_FATAL) {
	    return er;
	  }

	  if (r == 0 && er == EXT_SUCCESS) {
	    strncpy(done.applied, "[X]", sizeof(done.applied));
	  }
	}
      }

      break;

    case '7':
      return r;
    }
  }
}
