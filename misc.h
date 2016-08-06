#pragma once
#pragma once
#if !defined(STRICT)
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN

#pragma warning(disable: 4820) // '...' bytes padding added after data member
#pragma warning(disable: 4668) // '...' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable: 4917) // '...' : a GUID can only be associated with a class, interface or namespace
#pragma warning(disable: 4987) // nonstandard extension used: 'throw (...)'
#pragma warning(disable: 4365) // argument : conversion from 'int' to 'size_t', signed/unsigned mismatch
#pragma warning(error:   4706) // assignment in conditional expression (this is the only way I found to turn it on)

#if defined(_DEBUG) || defined(DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <Windows.h>
#include <Windowsx.h>
#include <Objbase.h>
#include <comdef.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#ifndef __cplusplus
#define bool BOOL
#define true TRUE
#define false FALSE
#endif
#define byte BYTE
#include <assert.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <tchar.h>
#include <intrin.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include <strmif.h> // for IMediaSample
#include <dshow.h>
#include <Control.h>
#include <Dvdmedia.h>
#include <Dwmapi.h>

#pragma comment(lib, "Dwmapi")
#pragma comment(lib, "strmiids")

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG _DEBUG
#endif
#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG DEBUG
#endif

#pragma warning(disable:  4711) // function selected for automatic inline expansion
#pragma warning(disable:  4100) // unreferenced formal param
#pragma warning(disable:  4800) // 'BOOL' : forcing value to bool 'true' or 'false'
#pragma warning(disable:  4996) // This function or variable may be unsafe. Consider using *_s instead.
#pragma warning(suppress: 6262) // excessive stack usage
#pragma warning(suppress: 6263) // using alloca in a loop
#pragma warning(suppress: 6255) //_alloca indicates failure by raising a stack overflow exception
#pragma warning(disable:  4365) // argument : conversion from 'int' to 'size_t', signed/unsigned mismatch
#pragma warning(disable:  4826) // Conversion from 'void *' to 'DWORD64' is sign-extended. This may cause unexpected runtime behavior.
#pragma warning(disable:  4505) // '...' : unreferenced local function has been removed

#define null NULL
#define countof(a) (sizeof(a) / sizeof((a)[0]))
#define print printf
#define thread_local_storage __declspec(thread)
#define OK(hr) SUCCEEDED(hr)

#ifdef __cplusplus
template <class T> struct com_t { // simply stupid ->Release wrapper for local COM pointers
    com_t() : p(0) { } // for more complicated cases use brains
    com_t(T* r) : p(r) { } 
    virtual ~com_t() { if (p != 0) { p->Release(); p = 0; } }
    operator T*() const throw() { return p; }
    T&  operator*() const { assert(p != 0); return *p; }
    T** operator&() throw() { assert(p == 0); return &p; }
    T*  operator->() const throw() { assert(p != 0); return p; }
private:
    T* p;
};

template<class T> inline int com_release(T* &r) { 
    if (r != 0) { 
        T* hold_on_to = r; 
        r = 0; // set reference to null before we call Release() to prevent possible infinite recursion in non-empty dtor
        return hold_on_to->Release(); 
    } else {
        return 0;
    }
}

#define if_failed_return_result(result) { HRESULT _hr_ = (result); if (FAILED(_hr_)) { print("%s failed: 0x%08X %s\n", #result, _hr_, strwinerr(_hr_)); errno = _hr_; return _hr_; } }
#define if_failed_return(result, r) { HRESULT _hr_ = (result); if (FAILED(_hr_)) { print("%s failed: 0x%08X %s\n", #result, _hr_, strwinerr(_hr_)); errno = _hr_; return r; } }
#define if_failed_return_false(result) if_failed_return(result, false);
#define if_failed_return_null(result) if_failed_return(result, null);

#define if_null_return(exp, res) { if ((exp) == null) { return res; } }
#define if_null_return_false(exp) if_null_return(exp, false);

#define if_false_return(exp, res) { if (!(exp)) { return res; } }
#define if_false_return_false(exp) if_false_return(exp, false);

#define if_false_return_null(exp) if_false_return(exp, null);


#if defined(DEBUG) && !defined(DEBUG_NEW)
   #ifndef DBG_NEW
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
      #define new DBG_NEW
   #endif
#endif // DEBUG

#ifdef __cplusplus
extern "C" {
#endif

const char* strwinerr(int e);
const char* strerr(int e);
int _utf8_for_wcs_(const wchar_t* wcs);
char* _wcs2str_(char* s, const wchar_t* wcs);

#define stack_alloc(bytes) alloca(bytes)
#define wcs2str(wcs) _wcs2str_((char*)stack_alloc(_utf8_for_wcs_(wcs) + 1), wcs)
#define str2wcs(s)   _str2wcs_((WCHAR*)stack_alloc((_wcs_for_utf8_(s) + 1) * sizeof(WCHAR)), s)

static inline uint32_t swap_bytes(uint32_t v) {
    return ((v >>  0) & 0xFF) << 24 | ((v >>  8) & 0xFF) << 16 | ((v >> 16) & 0xFF) <<  8 | ((v >> 24) & 0xFF) <<  0;
}

static inline int indexof(const int* a, int n, int v) {
    for (int i = 0; i < n; i++) { if (a[i] == v) { return i; } }
    return -1;
}


#ifdef __cplusplus
} // extern "C"
#endif

#endif

