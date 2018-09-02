#pragma once

#include "../Error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MFF_ERROR_INVALID_ARGUMENTS		0x0701
#define MFF_ERROR_FILE_NOT_FOUND		0x0702
#define MFF_ERROR_INTERNAL_ERROR		0x0703
#define MFF_ERROR_NOT_SUPPORTED			0x0704
#define MFF_ERROR_NO_REGISTER_ENTRIES	0x0705
#define MFF_ERROR_TYPE_NOT_REGISTERED	0x0706
#define MFF_ERROR_PATH_TOO_BIG			0x0707
#define MFF_ERROR_EXTENSION_TOO_BIG		0x0708
#define MFF_ERROR_ARCHIVE_NOT_FOUND		0x0709
#define MFF_ERROR_NOT_A_FILE			0x070A

#ifdef __cplusplus
}
#endif
