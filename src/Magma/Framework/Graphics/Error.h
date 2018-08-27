#pragma once

#include "../Error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MFG_ERROR_INVALID_ARGUMENTS			0x0501
#define MFG_ERROR_ALLOCATION_FAILED			0X0502
#define MFG_ERROR_NOT_FOUND					0x0503
#define MFG_ERROR_INVALID_DATA				0x0504
#define MFG_ERROR_UNSUPPORTED_MAJOR_VER		0x0505
#define MFG_ERROR_UNSUPPORTED_MINOR_VER		0x0506
#define MFG_ERROR_FAILED_TO_WRITE			0x0507
#define MFG_ERROR_NOT_SUPPORTED				0x0508
#define MFG_ERROR_NO_EXTENSION				0x0509
#define MFG_ERROR_INTERNAL					0x050A
#define MFG_ERROR_NO_REGISTER_ENTRIES		0x050B
#define MFG_ERROR_TYPE_NOT_REGISTERED		0x050C
#define MFG_ERROR_TOKENS_OVERFLOW			0x050D
#define MFG_ERROR_UNKNOWN_TOKEN				0x050E
#define MFG_ERROR_TOKEN_ATTRIBUTE_TOO_BIG	0x050F
#define MFG_ERROR_NODES_OVERFLOW			0x0510
#define MFG_ERROR_UNEXPECTED_TOKEN			0x0511
#define MFG_ERROR_UNEXPECTED_EOF			0x0512
#define MFG_ERROR_INACTIVE_NODE				0x0513
#define MFG_ERROR_FAILED_TO_PARSE			0x0514

#ifdef __cplusplus
}
#endif
