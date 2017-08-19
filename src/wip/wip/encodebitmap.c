
#include "pagegfx.h"
#include "encodebitmap.h"

#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>    
#elif __linux__
    #include <sys/stat.h>
#endif

#ifdef _WIN32

time_t convert2nixtime(FILETIME x) {
    time_t placeholder = 0;
    return placeholder;
}

time_t Getmodifiedtime_w32(const char *fn) {
    HANDLE hfile = CreateFile(fn.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    bool is_ok = true;
    time_t output;
    if (hfile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Unable to open file \n");
        is_ok = false;
    } else {
        FILETIME created, accessed, modified, toconvert;
        GetFileTime(hfile, &created, &accessed, &modified);
        if (modified.dwHighDateTime == 0) {
            output = convert2nixtime(created);
        } else {
            output = convert2nixtime(modified);
        }
    }
    return (is_ok) ? output : 0;
}

unsigned long Getfilesize_w32(const char *fn) {
    HANDLE hfile = CreateFile(fn.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    unsigned long output = 0;
    unsigned long x = 0;
    output = GetFileSize(hfile, (LPDWORD) &x);
    if (output == 0
        || output == INVALID_FILE_SIZE
        || output > MAXSIZE
        || x != 0) {

        fprintf(stderr, "Invalid file size \n");
    }
    return output;
}

#else

time_t Getmodifiedtime_linux(const char *fn) {
    ;
}

unsigned long Getfilesize_linux() {
    ;
}

#endif

time_t Getfilemodifiedtime(const char *fn) {
    if (fn != NULL) {
    #ifdef _WIN32
        return Getmodifiedtime_w32(fn);
    #else
        return Getmodifiedtime_linux(fn);
    #endif
    } else {
        return 0;
    }
}

unsigned long Getfilesize(const char *fn) {
    if (fn != NULL) {
    #ifdef _WIN32
        return Getfilesize_w32(fn);
    #else
        return Getfilesize_linux(fn);
    #endif
    } else {
        return 0;
    }
}



void encodebitmap(const char *ifile, 
                  const char *ofile,
                  int dpi, 
                  int dotsize, 
                  int redundancy, 
                  int border, 
                  int header) {
                      ;
}
