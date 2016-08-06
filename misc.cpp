#include "misc.h"

#ifdef __cplusplus
extern "C" {
#endif

static thread_local_storage char error_buffer[2 * 1024];

const char* strwinerr(int e) {
    int save = errno;
    int gle = GetLastError();
    error_buffer[0] = 0;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                    null, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                    error_buffer, sizeof(error_buffer), null);
    error_buffer[countof(error_buffer) - 1] = 0;
    char* cr = strrchr(error_buffer, '\r');
    if (cr != null) { *cr = 0; }
    char* cn = strrchr(error_buffer, '\n');
    if (cn != null) { *cn = 0; }
    errno = save;
    SetLastError(gle);
    return error_buffer;
}

const char* strerr(int e) {
    int save = errno;
    int gle = GetLastError();
    error_buffer[0] = 0;
    const char* res;
    if (e >= 0 && e <= EWOULDBLOCK) { // try posix errors first
        errno_t en = strerror_s(error_buffer, countof(error_buffer), e);
        error_buffer[countof(error_buffer) - 1] = 0;
        res = en != 0 || error_buffer[0] == 0 ? strwinerr(e) : error_buffer;                
    } else {
        res = strwinerr(e);
    }
    errno = save;
    SetLastError(gle);
    return res;
}

int _utf8_for_wcs_(const wchar_t* wcs) {
    int required_bytes_count = WideCharToMultiByte(
        CP_UTF8, // codepage
        WC_ERR_INVALID_CHARS, // flags
        wcs,
        -1,   // -1 means "wcs" is 0x0000 terminated
        null,
        0,
        null, //  default char*
        null  //  bool* used default char
    );
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

char* _wcs2str_(char* s, const wchar_t* wcs) {
    int r = WideCharToMultiByte(
        CP_UTF8, // codepage,
        WC_ERR_INVALID_CHARS, // flags,
        wcs,
        -1,   // -1 means 0x0000 terminated
        s,
        _utf8_for_wcs_(wcs),
        null, //  default char*,
        null  // bool* used default char
    );
    if (r == 0) {
        print("WideCharToMultiByte failed %d 0x%08X %s", strwinerr(GetLastError()));
    }
    return s;
}


#ifdef __cplusplus
} // extern "C"
#endif

