/* XQF - Quake server browser and launcher
XQF CC0 Source Code.
Copyright (C) 2025 XQF Team - https://xqf.github.io
No rights reserved. */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if defined(USE_RELATIVE_PREFIX)
#define XQF_MAX_PATH 1024
extern char xqf_PACKAGE_DATA_DIR[ XQF_MAX_PATH ];
extern char xqf_LOCALEDIR[ XQF_MAX_PATH ];
#else
extern const char* xqf_PACKAGE_DATA_DIR;
extern const char* xqf_LOCALEDIR;
#endif

#if defined(USE_RELATIVE_PREFIX)
EXTERNC void setDefaultDirs();
#endif

#undef EXTERNC
#endif // __SYSTEM_H__
