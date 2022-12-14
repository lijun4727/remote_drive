#pragma once

#define VERSION_MAJOR    1
#define VERSION_MINOR    0
#define VERSION_BUILD    4
#define VERSION_REVISION 14

#ifndef _T
#if !defined(ISPP_INVOKED) && (defined(UNICODE) || defined(_UNICODE))
#define _T(text) L##text
#else
#define _T(text) text
#endif
#endif

#define DO_STRINGIFY(x) _T(#x)
#define STRINGIFY(x)    DO_STRINGIFY(x)

#define VERSION_APP_NAME             STRINGIFY(远程驾驶2.0)
#define VERSION_DISPLAY              STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR)
#define VERSION_FILEVERSION_NUM      VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD,VERSION_REVISION
#define VERSION_FILEVERSION          STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_BUILD) "." STRINGIFY(VERSION_REVISION)
#define VERSION_COMPANYNAME          STRINGIFY(上海木蚁机器人科技有限公司)
#define VERSION_LEGALCOPYRIGHT       "Copyright 2022 " VERSION_COMPANYNAME " All Rights Reserved"
