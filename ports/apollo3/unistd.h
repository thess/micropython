#ifndef _LOCAL_UNISTD_H
#define _LOCAL_UNISTD_H

#include <stddef.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// We need a defintion of ssize_t for Crossworks
#ifndef ssize_t
#ifdef __RAL_SIZE_T
#define unsigned signed
typedef __RAL_SIZE_T ssize_t;
#undef unsigned
#endif
#else
typedef long ssize_t;
#endif

// Other unistd.h defintions needed
#define  SEEK_SET 0
#define  SEEK_CUR 1
#define  SEEK_END 2

// From math.h
#ifndef nanf
#define nanf __builtin_nanf
#endif


#ifdef __cplusplus
}
#endif

#endif // _LOCAL_UNISTD_H
